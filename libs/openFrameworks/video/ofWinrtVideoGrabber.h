#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "ofBaseTypes.h"
#include "ofPixels.h"
#include "ofEvents.h"
#include "CaptureFrameGrabber/CaptureFrameGrabber.h"

#include <collection.h>
#include <ppltasks.h>
#include <agile.h>
#include <mutex>
#include <memory>
#include <atomic>


class ofWinrtVideoGrabber : public ofBaseVideoGrabber
{

public:
	ofWinrtVideoGrabber();
	virtual ~ofWinrtVideoGrabber();

	vector<ofVideoDevice>	listDevices() const;

    bool					setup(int w, int h);
	void					update();
	bool					isFrameNew() const;
	bool					isInitialized() const;

	bool					setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat			getPixelFormat() const;

	ofPixels& 	    		getPixels();
	const ofPixels&			getPixels() const;

	void					close();
	void					clearMemory();

	void					videoSettings();

	float					getWidth() const;
	float					getHeight() const;

	void					setVerbose(bool bTalkToMe);
	void					setDeviceID(int deviceID);
	void					setDesiredFrameRate(int framerate);

    void                    appResume(ofAppResumeEventArgs &e);
    void                    appSuspend(ofAppSuspendEventArgs &e);


//protected:
	
	bool					bChooseDevice;
	bool 					bVerbose;
    bool                    bFlipImageX;
	std::atomic<bool>       bGrabberInited;
	int						m_deviceID;
    int						attemptFramerate;
    std::atomic<bool>       bIsFrameNew;
	int						width, height;
    int                     bytesPerPixel;
    unsigned long           frameCounter;
    unsigned long           currentFrame;

private:
    void                    _GrabFrameAsync(Media::CaptureFrameGrabber^ frameGrabber);
    void                    closeCaptureDevice();

    std::vector <ofVideoDevice> listDevicesTask() const;

    void                    SwapBuffers();

    std::unique_ptr<ofPixels>   m_frontBuffer;
    std::unique_ptr<ofPixels>   m_backBuffer;
    Platform::Agile<Windows::Devices::Enumeration::DeviceInformationCollection> m_devices;

    ::Media::CaptureFrameGrabber^ m_frameGrabber;
    std::mutex              m_mutex;
};