#include "ofWinrtVideoGrabber.h"
#include "ofUtils.h"
#include "ofEvents.h"

#if defined (TARGET_WINRT)

#include <ppltasks.h>
#include <ppl.h>
#include <agile.h>
#include <future>
#include <vector>
#include <atomic>


using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Windows::Media::Devices;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Devices::Enumeration;
using namespace Platform;
using namespace Windows::Foundation;


ofWinrtVideoGrabber::ofWinrtVideoGrabber()
    : m_frameGrabber(nullptr)
    , m_deviceID(0)
{

    // common
    bIsFrameNew = false;
    bVerbose = false;
    bFlipImageX = false;
    bGrabberInited = false;
    bChooseDevice = false;
    width = 320;	// default setting
    height = 240;	// default setting
    bytesPerPixel = 3;
    attemptFramerate = -1;

    ofAddListener(ofEvents().appSuspend, this, &ofWinrtVideoGrabber::appSuspend, ofEventOrder::OF_EVENT_ORDER_BEFORE_APP);

}

ofWinrtVideoGrabber::~ofWinrtVideoGrabber()
{
    close();
    ofRemoveListener(ofEvents().appResume, this, &ofWinrtVideoGrabber::appResume);
    ofRemoveListener(ofEvents().appSuspend, this, &ofWinrtVideoGrabber::appSuspend);
}

bool ofWinrtVideoGrabber::setup(int w, int h)
{
    width = w;
    height = h;
    bytesPerPixel = 3;
    bGrabberInited = false;

    m_frontBuffer = std::unique_ptr<ofPixels>(new ofPixels);
    m_backBuffer = std::unique_ptr<ofPixels>(new ofPixels);
    m_frontBuffer->allocate(w, h, bytesPerPixel);
    m_backBuffer->allocate(w, h, bytesPerPixel);
    frameCounter = 0;
    currentFrame = 0;

    if (bChooseDevice){
        bChooseDevice = false;
        ofLogNotice("ofWinrtVideoGrabber") << "initGrabber(): choosing " << m_deviceID;
    }
    else {
        m_deviceID = 0;
    }

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->StreamingCaptureMode = StreamingCaptureMode::Video; // Video-only capture

    // we need to have at least one video device
    // call listdevices() to find the default (first) video device
    if (!m_devices.Get()) 
    {
        listDevices();
        if (!m_devices.Get()) 
        {
            ofLogError("ofWinrtVideoGrabber") << "no video devices are available";
            return false;
        }
    }

    auto devInfo = m_devices.Get()->GetAt(m_deviceID);
    settings->VideoDeviceId = devInfo->Id;

    auto location = devInfo->EnclosureLocation;
    if (location != nullptr && location->Panel == Windows::Devices::Enumeration::Panel::Front)
    {
        bFlipImageX = true;
    }

    auto capture = ref new MediaCapture();
    create_task(capture->InitializeAsync(settings)).then([this, capture](){

        auto props = safe_cast<VideoEncodingProperties^>(capture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));
        props->Subtype = MediaEncodingSubtypes::Rgb24; 
        props->Width = width;
        props->Height = height;

        return ::Media::CaptureFrameGrabber::CreateAsync(capture, props);

    }).then([this](::Media::CaptureFrameGrabber^ frameGrabber)
    {
        m_frameGrabber = frameGrabber;
        bGrabberInited = true;
        _GrabFrameAsync(frameGrabber);
        ofAddListener(ofEvents().appResume, this, &ofWinrtVideoGrabber::appResume, ofEventOrder::OF_EVENT_ORDER_AFTER_APP);
    });

    return true;
}

void ofWinrtVideoGrabber::_GrabFrameAsync(Media::CaptureFrameGrabber^ frameGrabber)
{
    create_task(frameGrabber->GetFrameAsync()).then([this, frameGrabber](const ComPtr<IMF2DBuffer2>& buffer)
    {
        // do the RGB swizzle while copying the pixels from the IMF2DBuffer2
        BYTE *pbScanline;
        LONG plPitch;
        unsigned int numBytes = width * bytesPerPixel;
        CHK(buffer->Lock2D(&pbScanline, &plPitch));

        if (bFlipImageX) 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            uint8_t* buf = m_backBuffer->getData();

            for (unsigned int row = 0; row < height; row++)
            {
                unsigned int i = 0;
                unsigned int j = numBytes - 1;

                while (i < numBytes)
                {
                    // reverse the scan line
                    // as a side effect this also swizzles R and B channels
                    buf[j--] = pbScanline[i++];
                    buf[j--] = pbScanline[i++];
                    buf[j--] = pbScanline[i++];
                }
                pbScanline += plPitch;
                buf += numBytes;
            }
        } 
        else 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            uint8_t* buf = m_backBuffer->getData();

            for (unsigned int row = 0; row < height; row++)
            {
                for (unsigned int i = 0; i < numBytes; i += bytesPerPixel)
                {
                    // swizzle the R and B values (BGR to RGB)
                    buf[i] = pbScanline[i + 2];
                    buf[i + 1] = pbScanline[i + 1];
                    buf[i + 2] = pbScanline[i];
                }
                pbScanline += plPitch;
                buf += numBytes;
            }
        }
        CHK(buffer->Unlock2D());

        frameCounter++;

        if (bGrabberInited)
        {
            _GrabFrameAsync(frameGrabber);
        }
    }, task_continuation_context::use_current());
}



bool ofWinrtVideoGrabber::setPixelFormat(ofPixelFormat pixelFormat)
{
    //note as we only support RGB we are just confirming that this pixel format is supported
    if (pixelFormat == OF_PIXELS_RGB)
    {
        return true;
    }
    ofLogWarning("ofWinrtVideoGrabber") << "setPixelFormat(): requested pixel format not supported";
    return false;
}

ofPixelFormat ofWinrtVideoGrabber::getPixelFormat() const
{
    //note if you support more than one pixel format you will need to return a ofPixelFormat variable. 
    return OF_PIXELS_RGB;
}


std::string PlatformStringToString(Platform::String^ s) {
    std::wstring t = std::wstring(s->Data());
    return std::string(t.begin(), t.end());
}

vector <ofVideoDevice> ofWinrtVideoGrabber::listDevicesTask() const
{
    std::atomic<bool> ready(false);

    auto settings = ref new MediaCaptureInitializationSettings();

    vector <ofVideoDevice> devices;

    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([this, &devices, &ready](task<DeviceInformationCollection^> findTask)
    {
		const_cast<ofWinrtVideoGrabber*>(this)->m_devices = findTask.get();

        for (size_t i = 0; i < m_devices->Size; i++)
        {
            ofVideoDevice deviceInfo;
            auto d = m_devices->GetAt(i);
            deviceInfo.bAvailable = true;
            deviceInfo.deviceName = PlatformStringToString(d->Name);
            deviceInfo.hardwareName = deviceInfo.deviceName;
            devices.push_back(deviceInfo);
        }

        ready = true;
    });

    // wait for async task to complete
    int count = 0;
    while (!ready)
    {
        count++;
    }

    return devices;
}


//--------------------------------------------------------------------
vector<ofVideoDevice> ofWinrtVideoGrabber::listDevices() const
{
    // synchronous version of listing video devices on WinRT
    // not a recommended practice but oF expects synchronous device enumeration
    std::future<vector <ofVideoDevice>> result = std::async(std::launch::async, &ofWinrtVideoGrabber::listDevicesTask, this);
    return result.get();
}


//--------------------------------------------------------------------

void ofWinrtVideoGrabber::update()
{
    if (bGrabberInited == true)
    {
        SwapBuffers();
    }
}

void ofWinrtVideoGrabber::SwapBuffers()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (currentFrame != frameCounter)
    {
        bIsFrameNew = true;
        currentFrame = frameCounter;
        std::swap(m_backBuffer, m_frontBuffer);
    }
    else
    {
        bIsFrameNew = false;
    }
}

void ofWinrtVideoGrabber::appResume(ofAppResumeEventArgs &e)
{
    if (m_frameGrabber == nullptr)
    {
        ofRemoveListener(ofEvents().appResume, this, &ofWinrtVideoGrabber::appResume);
        setup(width, height);
    }
}

void ofWinrtVideoGrabber::appSuspend(ofAppSuspendEventArgs &e)
{
    closeCaptureDevice();
}


void ofWinrtVideoGrabber::closeCaptureDevice()
{
    bGrabberInited = false;
    if (m_frameGrabber != nullptr)
    {
        m_frameGrabber = nullptr;
#if 0
        m_frameGrabber->FinishAsync().then([this]() {
            m_frameGrabber = nullptr;
        });
#endif // 0

    }
}

void ofWinrtVideoGrabber::close()
{
    bGrabberInited = false;
    clearMemory();
 
}

void ofWinrtVideoGrabber::clearMemory()
{
    m_frontBuffer->clear();
}

ofPixels& ofWinrtVideoGrabber::getPixels()
{
    return *m_frontBuffer.get();
}

const ofPixels& ofWinrtVideoGrabber::getPixels() const
{
    return *m_frontBuffer.get();
}

float ofWinrtVideoGrabber::getWidth() const
{
    return width;
}

float ofWinrtVideoGrabber::getHeight() const
{
    return height;
}

bool ofWinrtVideoGrabber::isFrameNew() const
{
    return bIsFrameNew;
}

bool ofWinrtVideoGrabber::isInitialized() const {
	return bGrabberInited;
}

void ofWinrtVideoGrabber::setVerbose(bool bTalkToMe)
{
    bVerbose = bTalkToMe;
}

void ofWinrtVideoGrabber::setDeviceID(int deviceID)
{
    m_deviceID = deviceID;
    bChooseDevice = true;
}

void ofWinrtVideoGrabber::setDesiredFrameRate(int framerate)
{
    attemptFramerate = framerate;
}

void ofWinrtVideoGrabber::videoSettings(void)
{

    if (bGrabberInited == true)
    {
        m_frameGrabber->ShowCameraSettings();
    }
}

#endif
