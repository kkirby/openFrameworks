#pragma once

#include "ofConstants.h"
#include "ofBaseTypes.h"
#include "ofPixels.h"

#ifdef OF_VIDEO_PLAYER_MEDIAFOUNDATION

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <wrl.h>
#include <mfmediaengine.h>
#include <d3d11.h>

class MediaEngineNotifyCallback
{
public:
    virtual void OnMediaEngineEvent(DWORD meEvent) = 0;
    virtual ~MediaEngineNotifyCallback() {};
};


class ofMediaFoundationPlayer : public ofBaseVideoPlayer, public MediaEngineNotifyCallback
{

public:

    ofMediaFoundationPlayer();
    ~ofMediaFoundationPlayer();

    virtual void OnMediaEngineEvent(DWORD meEvent);

    bool			load(string name);
    void			closeMovie();
    void			close();
    void			update();

    void			play();
    void			stop();

    void			clearMemory();

    bool            setPixelFormat(ofPixelFormat pixelFormat);
    ofPixelFormat   getPixelFormat() const;

    bool 			isFrameNew() const;
    ofPixels& getPixels();
    const ofPixels&	getPixels() const;

    float 			getWidth() const;
    float 			getHeight() const;

    bool			isPaused() const;
    bool			isLoaded() const;
    bool			isPlaying() const;

    float 			getPosition();
    float 			getDuration();
    int			    getTotalNumFrames();
    float			getSpeed();
    bool			getIsMovieDone();
    ofLoopType 	    getLoopState();

    void 			setPosition(float pct);
    void 			setVolume(float volume);
    void 			setLoopState(ofLoopType state);
    void   		    setSpeed(float speed);
    void			setFrame(int frame);  // frame 0 = first frame...
    void 			setPaused(bool bPause);

    int			    getCurrentFrame();

    void			firstFrame();
    void			nextFrame();
    void			previousFrame();

    bool            bHavePixelsChanged;



protected:
    IMFSourceReader *   m_pReader;

    void Initialize();
    void SetURL(Platform::String^ szURL);
    void SetBytestream(Windows::Storage::Streams::IRandomAccessStream^ streamHandle);
    void MFPlay();
    void MFPause();

    void MFSetPlaybackPosition(float pos);
    double MFGetPlaybackPosition();
    void MFGetDuration(double *pDuration, BOOL *pbCanSeek);
    void MFFrameStep(BOOL forward);
    void MFMute(BOOL mute);
    void MFLoop(BOOL loop);
    void MFSetBalance(float fBal);
    BOOL MFIsSeeking();
    void MFSetVolume(float fVol);
    void MFGetNativeVideoSize(DWORD *cx, DWORD *cy) const;
    void MFRate(float rate);

    // Media Engine related
    Microsoft::WRL::ComPtr<IMFMediaEngine>       m_spMediaEngine;
    Microsoft::WRL::ComPtr<IMFMediaEngineEx>     m_spEngineEx;
	Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> m_spDXGIManager;
	Microsoft::WRL::ComPtr<ID3D11Device>         m_spDX11Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>  m_spDX11DeviceContext;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>      m_spReadTexture;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>      m_spWriteTexture;

    void start();

    ofPixels		 	pixels;
    int					width, height;
    bool				bLoaded;

    //these are public because the ofMediaFoundationPlayer implementation has some callback functions that need access
    //todo - fix this

    int					nFrames;				// number of frames
    bool				allocated;				// so we know to free pixels or not

    ofLoopType			currentLoopState;
    bool 				bStarted;
    bool 				bPlaying;
    bool 				bPaused;
    bool 				bEOF;
    bool 				bIsFrameNew;			// if we are new
    float				speed;
    
    BSTR                m_bstrURL;


};

#endif





