#include "ofMediaFoundationPlayer.h"
#include "ofUtils.h"

#include <algorithm>
#include <ppltasks.h>
#include <Strsafe.h>
#include <string>
#include <DXGIFormat.h>
#include <Wincodec.h>
#include <d3d11.h>
#include <dxgi.h>

using namespace Windows::Storage;
using namespace Platform;
using namespace concurrency;
using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;

static bool sMFInitialized = false;

#define ME_CAN_SEEK 0x00000002


namespace MEDIA
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw Platform::Exception::CreateException(hr);
        }
    }
}



// MediaEngineNotify: Implements the callback for Media Engine event notification.
class MediaEngineNotify : public IMFMediaEngineNotify
{
    long m_cRef;
    MediaEngineNotifyCallback* m_pCB;

public:
    MediaEngineNotify() : m_cRef(1), m_pCB(nullptr)
    {
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (__uuidof(IMFMediaEngineNotify) == riid)
        {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
        }
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        AddRef();

        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    // EventNotify is called when the Media Engine sends an event.
    STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR param1, DWORD param2)
    {
        if (meEvent == MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE)
        {
            SetEvent(reinterpret_cast<HANDLE>(param1));
        }
        else
        {
            m_pCB->OnMediaEngineEvent(meEvent);
        }

        return S_OK;
    }

    void MediaEngineNotifyCallback(MediaEngineNotifyCallback* pCB)
    {
        m_pCB = pCB;
    }
};


//---------------------------------------------------------------------------
ofMediaFoundationPlayer::ofMediaFoundationPlayer()
    : m_spMediaEngine(nullptr)
    , m_spEngineEx(nullptr)
    , m_pReader(nullptr)
{
    bLoaded = false;
    width = 0;
    height = 0;
    speed = 1;
    bStarted = false;
    nFrames = 0;
    bPaused = true;
    bEOF = false;
    currentLoopState = OF_LOOP_NORMAL;
}


//---------------------------------------------------------------------------
ofMediaFoundationPlayer::~ofMediaFoundationPlayer(){

    closeMovie();
    clearMemory();
}

//---------------------------------------------------------------------------
ofPixels& ofMediaFoundationPlayer::getPixels(){
    return pixels;
}

//---------------------------------------------------------------------------
const ofPixels& ofMediaFoundationPlayer::getPixels() const {
    return pixels;
}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::update(){

    if (bLoaded == true)
    {
        LONGLONG pts;
        bIsFrameNew = false;
        if (m_spMediaEngine->OnVideoStreamTick(&pts) == S_OK)
        {
            // ---------------------------------------------------
            // 		on all platforms,
            // 		do "new"ness ever time we idle...
            // 		before "isFrameNew" was clearning,
            // 		people had issues with that...
            // 		and it was badly named so now, newness happens
            // 		per-idle not per isNew call
            // ---------------------------------------------------
            HRESULT hr;
            DWORD w, h;
            MFGetNativeVideoSize(&w, &h);
            pixels.allocate(w, h, ofImageType::OF_IMAGE_COLOR_ALPHA);

			if(!m_spReadTexture)
			{
				D3D11_TEXTURE2D_DESC desc;
				desc.Width = w;
				desc.Height = h;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = D3D11_USAGE_STAGING;
				desc.BindFlags = 0;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.MiscFlags = 0;
				hr = m_spDX11Device->CreateTexture2D(&desc, nullptr, m_spReadTexture.GetAddressOf());
				if(FAILED(hr))
					return;

				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_RENDER_TARGET;
				desc.CPUAccessFlags = 0;
				hr = m_spDX11Device->CreateTexture2D(&desc, nullptr, m_spWriteTexture.GetAddressOf());
				if(FAILED(hr))
					return;
			}

            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = w;
            rect.bottom = h;
            MFARGB mfargb;
            memset(&mfargb, 0, sizeof(MFARGB));
            hr = m_spMediaEngine->TransferVideoFrame(m_spWriteTexture.Get(), nullptr, &rect, &mfargb);
            if(FAILED(hr))
                return;

			m_spDX11DeviceContext->CopyResource(m_spReadTexture.Get(), m_spWriteTexture.Get());
			m_spDX11DeviceContext->Flush();
			D3D11_MAPPED_SUBRESOURCE mapped;
			hr = m_spDX11DeviceContext->Map(m_spReadTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
			if(FAILED(hr))
				return;

			//need to swizzle red and blue channels
            unsigned char *pixelArray = pixels.getData();
			for(int y = 0; y < h; ++y)
			{
				int row = y * w;
				for(int x = 0; x < w; ++x)
				{
					int index = (row + x) * 4;
					BYTE *mappedData = static_cast<BYTE*>(mapped.pData);
					pixelArray[index + 0] = mappedData[index + 2];
					pixelArray[index + 1] = mappedData[index + 1];
					pixelArray[index + 2] = mappedData[index + 0];
					pixelArray[index + 3] = mappedData[index + 3];
				}
			}
			//memcpy(pixelArray, mapped.pData, pixels.size());
			m_spDX11DeviceContext->Unmap(m_spReadTexture.Get(), 0);
            if(FAILED(hr))
                return;

            bIsFrameNew = true;
        }
        else
        {
            bIsFrameNew = false;
        }
    }
}

//---------------------------------------------------------------------------
bool ofMediaFoundationPlayer::isFrameNew() const {
    return bIsFrameNew;
}
//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::close(){
    closeMovie();
}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::closeMovie(){

    if (bLoaded == true){
        if (m_pReader)
        {
            m_pReader->Release();
            m_pReader = nullptr;
        }
    }

    bLoaded = false;
}


//---------------------------------------------------------------------------
bool ofMediaFoundationPlayer::load(string name){

    HRESULT hr = S_OK;
 
    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------

    closeMovie();       // if we have a movie open, close it
    bLoaded = false;	// try to load now

    Initialize();

    if (SUCCEEDED(hr))
    {
        Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
        StorageFolder^ installedLocation = package->InstalledLocation;
        std::string path = "ms-appx:///data/" + name;
        std::wstring wpath(path.begin(), path.end());
        auto uri = ref new Windows::Foundation::Uri(ref new Platform::String(wpath.c_str()));
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        auto fileTask = create_task(StorageFile::GetFileFromApplicationUriAsync(uri), task_continuation_context::use_arbitrary());

        fileTask.then([this, eventHandle](StorageFile^ fileHandle)
        {
            try
            {
                if (!fileHandle)
                {
                    return;
                }
                SetURL(fileHandle->Path);
            }
            catch (Platform::Exception^)
            {
                // TODO handle exception correctly
                throw std::exception("can't get StorageFile");
            }
			HANDLE eventHandle2 = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            task<IRandomAccessStream^> fOpenStreamTask(fileHandle->OpenAsync(Windows::Storage::FileAccessMode::Read), task_continuation_context::use_arbitrary());

            try
            {
                auto vidPlayer = this;
                fOpenStreamTask.then([vidPlayer, eventHandle2](IRandomAccessStream^ streamHandle)
                {
                    try
                    {
                        vidPlayer->SetBytestream(streamHandle);
                    }
                    catch (Platform::Exception^)
                    {
						SetEvent(eventHandle2);
                        MEDIA::ThrowIfFailed(E_UNEXPECTED);
                    }
					SetEvent(eventHandle2);
                }, task_continuation_context::use_arbitrary());
            }
            catch (Platform::Exception^)
            {
                // TODO handle exception correctly
				CloseHandle(eventHandle2);
				SetEvent(eventHandle);
                throw std::exception("can't get IRandomAccessStream");
            }
			WaitForSingleObjectEx(eventHandle2, INFINITE, FALSE);
			CloseHandle(eventHandle2);
			SetEvent(eventHandle);
        }, task_continuation_context::use_arbitrary());
		WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
		CloseHandle(eventHandle);
    }

#if 0


    if (SUCCEEDED(hr))
    {
        std::string fullPath = ofFilePath::join(WinrtLocalDirPath(""), ofToDataPath(name));
        std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
        std::wstring path(fullPath.begin(), fullPath.end());
        hr = MFCreateSourceReaderFromURL(path.c_str(), NULL, &m_pReader);
    }
#endif // 0


#if 0


    if (name.substr(0, 7) == "http://" || name.substr(0, 7) == "rtsp://"){
        if (!createMovieFromURL(name, moviePtr)) return false;
    }
    else{
        name = ofToDataPath(name);
        if (!createMovieFromPath((char *) name.c_str(), moviePtr)) return false;
    }

    bool bDoWeAlreadyHaveAGworld = false;
    if (width != 0 && height != 0){
        bDoWeAlreadyHaveAGworld = true;
    }
    Rect 				movieRect;
    GetMovieBox(moviePtr, &(movieRect));
    if (bDoWeAlreadyHaveAGworld){
        // is the gworld the same size, then lets *not* de-allocate and reallocate:
        if (width == movieRect.right &&
            height == movieRect.bottom){
            SetMovieGWorld(moviePtr, offscreenGWorld, nil);
        }
        else {
            width = movieRect.right;
            height = movieRect.bottom;
            pixels.clear();
            delete [] offscreenGWorldPixels;
            if ((offscreenGWorld)) DisposeGWorld((offscreenGWorld));
            createImgMemAndGWorld();
        }
    }
    else {
        width = movieRect.right;
        height = movieRect.bottom;
        createImgMemAndGWorld();
    }

    if (moviePtr == NULL){
        return false;
    }

    //----------------- callback method
    myDrawCompleteProc = NewMovieDrawingCompleteUPP(DrawCompleteProc);
    SetMovieDrawingCompleteProc(moviePtr, movieDrawingCallWhenChanged, myDrawCompleteProc, (long)this);

    // ------------- get the total # of frames:
    nFrames = 0;
    TimeValue			curMovieTime;
    curMovieTime = 0;
    TimeValue			duration;

    //OSType whichMediaType	= VIDEO_TYPE; // mingw chokes on this
    OSType whichMediaType = FOUR_CHAR_CODE('vide');

    short flags = nextTimeMediaSample + nextTimeEdgeOK;

    while (curMovieTime >= 0) {
        nFrames++;
        GetMovieNextInterestingTime(moviePtr, flags, 1, &whichMediaType, cus



    // ------------- get some pixels in there ------
    GoToBeginningOfMovie(moviePtr);
    SetMovieActiveSegment(moviePtr, -1, -1);
    MoviesTask(moviePtr, 0);

#if defined(TARGET_OSX) && defined(__BIG_ENDIAN__)
    convertPixels(offscreenGWorldPixels, pixels.getPixels(), width, height);
#endif

    bStarted = false;
    bLoaded = true;
    bPlaying = false;
    bHavePixelsChanged = false;
    speed = 1;


    //--------------------------------------
#endif
    //--------------------------------------

#endif

    return true;
}


//--------------------------------------------------------
void ofMediaFoundationPlayer::start(){

    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------

    if (bLoaded == true && bStarted == false){
        setLoopState(currentLoopState);

        bHavePixelsChanged = true;

        bStarted = true;
        bPlaying = true;
    }

    //--------------------------------------
#endif
    //--------------------------------------
}

//--------------------------------------------------------
void ofMediaFoundationPlayer::play(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "play(): movie not loaded";
        return;
    }

    bPlaying = true;
    bPaused = false;

    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------

    if (!bStarted){
        start();
    }
    else {

    }

    //--------------------------------------
#endif
    //--------------------------------------

    //this is if we set the speed first but it only can be set when we are playing.
    setSpeed(speed);

}

//--------------------------------------------------------
void ofMediaFoundationPlayer::stop(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "stop(): movie not loaded";
        return;
    }

    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------


    bStarted = false;

    //--------------------------------------
#endif
    //--------------------------------------

    bPlaying = false;
}

//--------------------------------------------------------
void ofMediaFoundationPlayer::setVolume(float volume){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "setVolume(): movie not loaded";
        return;
    }

    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------

    //SetMovieVolume(moviePtr, volume * 255);

    //--------------------------------------
#endif
    //--------------------------------------

}


//--------------------------------------------------------
void ofMediaFoundationPlayer::setLoopState(ofLoopType state){


    if (isLoaded()){

        switch (state) {
        case OF_LOOP_NORMAL:
            MFLoop(true);
            break;
        case OF_LOOP_PALINDROME:
            break;
        case OF_LOOP_NONE:
        default:
            MFLoop(false);
            break;
        }
    }


    //store the current loop state;
    currentLoopState = state;

}

//---------------------------------------------------------------------------
ofLoopType ofMediaFoundationPlayer::getLoopState(){
    return currentLoopState;
}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::setPosition(float pct){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "setPosition(): movie not loaded";
        return;
    }

    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------


    //--------------------------------------
#endif
    //--------------------------------------


}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::setFrame(int frame){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "setFrame(): movie not loaded";
        return;
    }

    //--------------------------------------
#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION
    //--------------------------------------

    // frame 0 = first frame...

    // this is the simple way...
    //float durationPerFrame = getDuration() / getTotalNumFrames();

    // seems that freezing, doing this and unfreezing seems to work alot
    // better then just SetMovieTimeValue() ;



    //--------------------------------------
#endif
    //--------------------------------------

}


//---------------------------------------------------------------------------
float ofMediaFoundationPlayer::getDuration(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "getDuration(): movie not loaded";
        return 0.0;
    }

    double duration;
    BOOL canSeek;

    MFGetDuration(&duration, &canSeek);
    return (float)duration;
}

//---------------------------------------------------------------------------
float ofMediaFoundationPlayer::getPosition(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "getPosition(): movie not loaded";
        return 0.0;
    }

    double duration;
    BOOL canSeek;

    MFGetDuration(&duration, &canSeek);
    double position = MFGetPlaybackPosition();

    float pct = ((float) position / (float) duration);
    return pct;
}

//---------------------------------------------------------------------------
int ofMediaFoundationPlayer::getCurrentFrame(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "getCurrentFrame(): movie not loaded";
        return 0;
    }

    int frame = 0;

    // zach I think this may fail on variable length frames...
    float pos = getPosition();

    float  framePosInFloat = ((float) getTotalNumFrames() * pos);
    int    framePosInInt = (int) framePosInFloat;
    float  floatRemainder = (framePosInFloat - framePosInInt);
    if (floatRemainder > 0.5f) framePosInInt = framePosInInt + 1;
    //frame = (int)ceil((getTotalNumFrames() * getPosition()));
    frame = framePosInInt;

    return frame;


}

//---------------------------------------------------------------------------
bool ofMediaFoundationPlayer::setPixelFormat(ofPixelFormat pixelFormat){
    //note as we only support RGB we are just confirming that this pixel format is supported
    if (pixelFormat == OF_PIXELS_RGBA){
        return true;
    }
    ofLogWarning("ofMediaFoundationPlayer") << "setPixelFormat(): requested pixel format " << pixelFormat << " not supported, expecting OF_PIXELS_RGBA";
    return false;
}

//---------------------------------------------------------------------------
ofPixelFormat ofMediaFoundationPlayer::getPixelFormat() const {
    //note if you support more than one pixel format you will need to return a ofPixelFormat variable. 
    return OF_PIXELS_RGBA;
}


//---------------------------------------------------------------------------
bool ofMediaFoundationPlayer::getIsMovieDone(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "getIsMovieDone(): movie not loaded";
        return false;
    }

    return bEOF;

}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::firstFrame(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "firstFrame(): movie not loaded";
        return;
    }


    setFrame(0);

}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::nextFrame(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "nextFrame(): movie not loaded";
        return;
    }

    MFFrameStep(true);
}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::previousFrame(){
    if (!isLoaded()){
        ofLogError("ofMediaFoundationPlayer") << "previousFrame(): movie not loaded";
        return;
    }

    MFFrameStep(false);
}



//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::setSpeed(float _speed){

    speed = _speed;

    if (bPlaying == true){
        MFRate(speed);
    }

}

//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::setPaused(bool _bPause){

    bPaused = _bPause;

    if (_bPause == true)
    {
        MFPause();
    }
    else
    {
        MFPlay();
    }
}


//---------------------------------------------------------------------------
void ofMediaFoundationPlayer::clearMemory(){

    pixels.clear();

}

//---------------------------------------------------------------------------
float ofMediaFoundationPlayer::getSpeed(){
    return speed;
}

//------------------------------------
int ofMediaFoundationPlayer::getTotalNumFrames(){
    return nFrames;
}

//----------------------------------------------------------
float ofMediaFoundationPlayer::getWidth() const {

    DWORD w, h;
    MFGetNativeVideoSize(&w, &h);
    return (float) w;
}

//----------------------------------------------------------
float ofMediaFoundationPlayer::getHeight() const {
    DWORD w, h;
    MFGetNativeVideoSize(&w, &h);
    return (float) h;
}

//----------------------------------------------------------
bool ofMediaFoundationPlayer::isPaused() const {
    return bPaused;
}

//----------------------------------------------------------
bool ofMediaFoundationPlayer::isLoaded() const {
    return bLoaded;
}

//----------------------------------------------------------
bool ofMediaFoundationPlayer::isPlaying() const {
    return bPlaying;
}

// Set a URL
void ofMediaFoundationPlayer::SetURL(Platform::String^ szURL)
{
    if (nullptr != m_bstrURL)
    {
        ::CoTaskMemFree(m_bstrURL);
        m_bstrURL = nullptr;
    }

    size_t cchAllocationSize = 1 + ::wcslen(szURL->Data());
    m_bstrURL = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(cchAllocationSize));

    if (m_bstrURL == 0)
    {
        MEDIA::ThrowIfFailed(E_OUTOFMEMORY);
    }

    StringCchCopyW(m_bstrURL, cchAllocationSize, szURL->Data());

    return;
}

// Set Bytestream
void ofMediaFoundationPlayer::SetBytestream(IRandomAccessStream^ streamHandle)
{
    HRESULT hr = S_OK;
    ComPtr<IMFByteStream> spMFByteStream = nullptr;

    MEDIA::ThrowIfFailed(
        MFCreateMFByteStreamOnStreamEx((IUnknown*) streamHandle, &spMFByteStream)
        );

    MEDIA::ThrowIfFailed(
        m_spEngineEx->SetSourceFromByteStream(spMFByteStream.Get(), m_bstrURL)
        );

    return;
}

void ofMediaFoundationPlayer::OnMediaEngineEvent(DWORD meEvent)
{
    switch (meEvent)
    {
    case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
    {
        bEOF  = FALSE;
        bLoaded = true;
    }
        break;
    case MF_MEDIA_ENGINE_EVENT_CANPLAY:
    {
        // Start the Playback
        MFPlay();
    }
        break;
    case MF_MEDIA_ENGINE_EVENT_PLAY:
        //m_fPlaying = TRUE;
        break;
    case MF_MEDIA_ENGINE_EVENT_PAUSE:
        //m_fPlaying = FALSE;
        break;
    case MF_MEDIA_ENGINE_EVENT_ENDED:

        if (m_spMediaEngine->HasVideo())
        {
           // StopTimer();
        }
        //m_fEOS = TRUE;
        break;
    case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
        break;
    case MF_MEDIA_ENGINE_EVENT_ERROR:
        break;
    }

    return;
}

// Start playback.
void ofMediaFoundationPlayer::MFPlay()
{
    if (m_spMediaEngine)
    {
        setLoopState(currentLoopState);

        if (bEOF)
        {
            MFSetPlaybackPosition(0);
        }
        else
        {
            MEDIA::ThrowIfFailed(
                m_spMediaEngine->Play()
                );
        }

        bEOF = FALSE;
        bPlaying = TRUE;
        bPaused = FALSE;
    }
    return;
}

// Is the player in the middle of a seek operation?
BOOL ofMediaFoundationPlayer::MFIsSeeking()
{
    if (m_spMediaEngine)
    {
        return m_spMediaEngine->IsSeeking();
    }
    else
    {
        return FALSE;
    }
}


// Seek to a new playback position.
void ofMediaFoundationPlayer::MFSetPlaybackPosition(float pos)
{
    if (m_spMediaEngine)
    {
        MEDIA::ThrowIfFailed(
            m_spMediaEngine->SetCurrentTime(pos)
            );
    }
}

// Pause playback.
void ofMediaFoundationPlayer::MFPause()
{
    if (m_spMediaEngine)
    {
        MEDIA::ThrowIfFailed(
            m_spMediaEngine->Pause()
            );
    }
    return;
}

// Set the audio volume.
void ofMediaFoundationPlayer::MFSetVolume(float fVol)
{
    if (m_spMediaEngine)
    {
        MEDIA::ThrowIfFailed(
            m_spMediaEngine->SetVolume(fVol)
            );
    }
    return;
}

// Set the audio balance.
void ofMediaFoundationPlayer::MFSetBalance(float fBal)
{
    if (m_spEngineEx)
    {
        MEDIA::ThrowIfFailed(
            m_spEngineEx->SetBalance(fBal)
            );
    }
    return;
}

// Mute the audio.
void ofMediaFoundationPlayer::MFRate(float rate)
{
    if (m_spMediaEngine)
    {
        MEDIA::ThrowIfFailed(
            m_spMediaEngine->SetDefaultPlaybackRate(rate)
            );
    }
    return;
}


// Mute the audio.
void ofMediaFoundationPlayer::MFMute(BOOL mute)
{
    if (m_spMediaEngine)
    {
        MEDIA::ThrowIfFailed(
            m_spMediaEngine->SetMuted(mute)
            );
    }
    return;
}

// Loop the video
void ofMediaFoundationPlayer::MFLoop(BOOL loop)
{
    if (m_spMediaEngine)
    {
        MEDIA::ThrowIfFailed(
            m_spMediaEngine->SetLoop(loop)
            );
    }
    return;
}

// Step forward one frame.
void ofMediaFoundationPlayer::MFFrameStep(BOOL forward)
{
    if (m_spEngineEx)
    {
        MEDIA::ThrowIfFailed(
            m_spEngineEx->FrameStep(forward)
            );
    }
    return;
}

// Get the duration of the content.
void ofMediaFoundationPlayer::MFGetDuration(double *pDuration, BOOL *pbCanSeek)
{
    if (m_spMediaEngine)
    {
        double duration = m_spMediaEngine->GetDuration();

        // NOTE:
        // "duration != duration"
        // This tests if duration is NaN, because NaN != NaN

        if (duration != duration || duration == std::numeric_limits<float>::infinity())
        {
            *pDuration = 0;
            *pbCanSeek = FALSE;
        }
        else
        {
            *pDuration = duration;

            DWORD caps = 0;
            m_spEngineEx->GetResourceCharacteristics(&caps);
            *pbCanSeek = (caps & ME_CAN_SEEK) == ME_CAN_SEEK;
        }
    }
    else
    {
        MEDIA::ThrowIfFailed(E_FAIL);
    }

    return;
}

void ofMediaFoundationPlayer::MFGetNativeVideoSize(DWORD *cx, DWORD *cy) const
{
    if (m_spMediaEngine)
    {
        m_spMediaEngine->GetNativeVideoSize(cx, cy);
    }
    return;
}


// Get the current playback position.
double ofMediaFoundationPlayer::MFGetPlaybackPosition()
{
    if (m_spMediaEngine)
    {
        return m_spMediaEngine->GetCurrentTime();
    }
    else
    {
        return 0;
    }
}

// Create a new instance of the Media Engine.
void ofMediaFoundationPlayer::Initialize()
{
    ComPtr<IMFMediaEngineClassFactory> spFactory;
    ComPtr<IMFAttributes> spAttributes;
    ComPtr<MediaEngineNotify> spNotify;

    HRESULT hr = S_OK;


    MEDIA::ThrowIfFailed(MFStartup(MF_VERSION));

    //EnterCriticalSection(&m_critSec);

    try
    {
		D3D_FEATURE_LEVEL levels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,  
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		D3D_FEATURE_LEVEL featureLevel;

		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			levels,
			ARRAYSIZE(levels),
			D3D11_SDK_VERSION,
			m_spDX11Device.GetAddressOf(),
			&featureLevel,
			m_spDX11DeviceContext.GetAddressOf()
			);
		if(FAILED(hr))
			return;
        UINT resetToken;
        MEDIA::ThrowIfFailed(
            MFCreateDXGIDeviceManager(&resetToken, &m_spDXGIManager)
            );

        MEDIA::ThrowIfFailed(
            m_spDXGIManager->ResetDevice(m_spDX11Device.Get(), resetToken)
            );

        // Create our event callback object.
        spNotify = new MediaEngineNotify();
        if (spNotify == nullptr)
        {
            MEDIA::ThrowIfFailed(E_OUTOFMEMORY);
        }

        spNotify->MediaEngineNotifyCallback(this);

        // Create the class factory for the Media Engine.
        MEDIA::ThrowIfFailed(
            CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spFactory))
            );

        // Set configuration attribiutes.
        MEDIA::ThrowIfFailed(
            MFCreateAttributes(&spAttributes, 1)
            );
			
        MEDIA::ThrowIfFailed(
            spAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown*) m_spDXGIManager.Get())
            );

        MEDIA::ThrowIfFailed(
            spAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, (IUnknown*) spNotify.Get())
            );


        MEDIA::ThrowIfFailed(
            spAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM)
            );


        // Create the Media Engine.
        const DWORD flags = MF_MEDIA_ENGINE_WAITFORSTABLE_STATE;
        MEDIA::ThrowIfFailed(
            spFactory->CreateInstance(flags, spAttributes.Get(), &m_spMediaEngine)
            );

        MEDIA::ThrowIfFailed(
            m_spMediaEngine.Get()->QueryInterface(__uuidof(IMFMediaEngine), (void**) &m_spEngineEx)
            );

    }
    catch (Platform::Exception^)
    {
#if 0
        CoreWindowDialog^ coreWindowDialog = ref new CoreWindowDialog(\
            "Failed to initialize DirectX device.");
        task<Windows::UI::Popups::IUICommand^> coreWindowDialogTask(coreWindowDialog->ShowAsync());

        coreWindowDialogTask.then([this](Windows::UI::Popups::IUICommand^ uiCommand){
            m_fExitApp = TRUE;
        });
#endif // 0

    }

    //LeaveCriticalSection(&m_critSec);

    return;
}




