#pragma once

#include "ofConstants.h"
#include "ofBaseApp.h"
#include "ofAppBaseWindow.h"
#include "ofThread.h"
#include "ofAppRunner.h"

#include <queue>
#include <map>

class ofAppWinRTWindow : public ofAppBaseGLESWindow, public ofThread {
	public:
		
		ofAppWinRTWindow();
		virtual ~ofAppWinRTWindow();
		
		void setup(const ofGLESWindowSettings & settings);
		void update();
		void draw();
		ofCoreEvents& events();
		shared_ptr<ofBaseRenderer>& renderer();
		
		virtual glm::vec2 getWindowSize();
		virtual glm::vec2 getScreenSize();
		
		virtual int getWidth();
		virtual int getHeight();
		
		virtual void setOrientation(ofOrientation orientation);
		virtual ofOrientation getOrientation();
		
		virtual void enableSetupScreen();
		virtual void disableSetupScreen();
		
		static bool doesLoop(){ return true; }
		static bool allowsMultiWindow(){ return false; }
		static void loop(){};
		static bool needsPolling(){ return false; }
		static void pollEvents(){};

		void winrtSetupComplete(int width, int height);
		void runOnce();
		
		void OnPointerPressed(Windows::UI::Core::PointerEventArgs^ args, float dpi);
		void OnPointerMoved(Windows::UI::Core::PointerEventArgs^ args, float dpi);
		void OnPointerReleased(Windows::UI::Core::PointerEventArgs^ args, float dpi);
		void OnKeyPressed(Windows::UI::Core::KeyEventArgs^ args);
		void OnKeyReleased(Windows::UI::Core::KeyEventArgs^ args);
		void OnWindowSizeChanged(int width, int height);

	protected:
		bool isInited;
		ofRectangle currentWindowRect;
		ofGLESWindowSettings settings;
		ofOrientation orientation;

		int currentTouchIndex;
		int mouseInUse;
		bool bEnableSetupScreen;
		bool bMousePressed;
		ofCoreEvents coreEvents;

		map<int, int> touchInputTracker;
		queue<int> availableTouchIndices;

		shared_ptr<ofBaseRenderer> currentRenderer;
		
		void setWindowRect(const ofRectangle& requestedWindowRect);
		
		void rotateMouseXY(ofOrientation orientation, double &x, double &y);
		void NotifyTouchEvent(int id, ofEvent<ofTouchEventArgs>& touchEvents, Windows::UI::Core::PointerEventArgs^ args, float dpi);
	
	private:
		static ofAppWinRTWindow* instance;
};

