#include "ofAppWinRTWindow.h"
#include "ofEvents.h"
#include "ofGLProgrammableRenderer.h"
#include "ofGLRenderer.h"
#include "ofAppRunner.h"
#include "ofLog.h"
#include <Windows.h>
#include <queue>

#include <agile.h>
#include <ppltasks.h>
#include <d2d1_2.h>

const string ofGLRenderer::TYPE="GL";

ofAppWinRTWindow* ofAppWinRTWindow::instance = NULL;

void ofGLReadyCallback();

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

ofAppWinRTWindow::ofAppWinRTWindow()
	: ofAppBaseGLESWindow() 
{
	ofLogVerbose("ofAppWinRTWindow") << "Created.";
	if(instance != NULL){
		ofLogError("ofAppEGLWindow") << "trying to create more than one instance";
	}
	instance = this;
	bEnableSetupScreen = false;
	isInited = false;
	currentWindowRect.width = currentWindowRect.height = 1;
	currentWindowRect.x = currentWindowRect.y = 0;
	orientation = OF_ORIENTATION_DEFAULT;
	currentTouchIndex = 2;
	mouseInUse = 0;
}

ofAppWinRTWindow::~ofAppWinRTWindow(){
}

//------------------------------------------------------------
void ofAppWinRTWindow::setup(const ofGLESWindowSettings& _settings) {
	settings = _settings;
	
	bEnableSetupScreen = true;
	isInited  = false;
	
	currentRenderer = make_shared<ofGLProgrammableRenderer>(this);
}

void ofAppWinRTWindow::update(){
	events().notifyUpdate();
}

void ofAppWinRTWindow::draw(){
	currentRenderer->startRender();
	if(bEnableSetupScreen)currentRenderer->setupScreen();
	coreEvents.notifyDraw();
	currentRenderer->finishRender();
}

ofCoreEvents& ofAppWinRTWindow::events(){
	return coreEvents;
}

shared_ptr<ofBaseRenderer>& ofAppWinRTWindow::renderer(){
	return currentRenderer;
}

void ofAppWinRTWindow::enableSetupScreen(){
	bEnableSetupScreen = true;
}

void ofAppWinRTWindow::disableSetupScreen(){
	bEnableSetupScreen = false;
}

void ofAppWinRTWindow::runOnce(){
	if(isInited){
		update();
		draw();
	}
}

glm::vec2 ofAppWinRTWindow::getWindowSize(){
	return {currentWindowRect.width, currentWindowRect.height};
}

glm::vec2 ofAppWinRTWindow::getScreenSize(){
	return getWindowSize();
}

int ofAppWinRTWindow::getWidth(){
	return currentWindowRect.width;
}

int ofAppWinRTWindow::getHeight(){
	return currentWindowRect.height;
}

void ofAppWinRTWindow::setOrientation(ofOrientation orientationIn){
	orientation = orientationIn;
}

ofOrientation ofAppWinRTWindow::getOrientation(){
	return orientation;
}

void ofAppWinRTWindow::setWindowRect(const ofRectangle& requestedWindowRect){
	currentWindowRect.width = max(requestedWindowRect.width,1.0f);
	currentWindowRect.height = max(requestedWindowRect.height,1.0f);
	coreEvents.notifyWindowResized(currentWindowRect.width,	currentWindowRect.height);
}

// ---- Exports used by XAML

void ofAppWinRTWindow::winrtSetupComplete(int width,int height){
	isInited = true;
	setWindowRect({0.0f,0.0f,(float)width,(float)height});
	static_cast<ofGLProgrammableRenderer*>(currentRenderer.get())->setup(settings.glesVersion,0);
	/*ofSetupGraphicDefaults();
	ofBackground(200);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();*/
	events().notifySetup();
}

void ofAppWinRTWindow::rotateMouseXY(ofOrientation orientation, double &x, double &y) {
	int savedY;
	switch (orientation) {
	case OF_ORIENTATION_180:
		x = ofGetWidth() - x;
		y = ofGetHeight() - y;
		break;

	case OF_ORIENTATION_90_RIGHT:
		savedY = y;
		y = x;
		x = ofGetWidth() - savedY;
		break;

	case OF_ORIENTATION_90_LEFT:
		savedY = y;
		y = ofGetHeight() - x;
		x = savedY;
		break;

	case OF_ORIENTATION_DEFAULT:
	default:
		break;
	}
}

void ofAppWinRTWindow::NotifyTouchEvent(int id, ofEvent<ofTouchEventArgs>& touchEvents, PointerEventArgs^ args, float dpi)
{
	ofTouchEventArgs touchEventArgs;
	PointerPoint^ pointerPoint = args->CurrentPoint;
	Point point = pointerPoint->Position;
	PointerPointProperties^ props = pointerPoint->Properties;

	touchEventArgs.x = point.X * dpi;
	touchEventArgs.y = point.Y * dpi;
	touchEventArgs.type = ofTouchEventArgs::doubleTap;
	touchEventArgs.id = id;
	touchEventArgs.pressure = props->Pressure;
	touchEventArgs.numTouches = touchInputTracker.size();

	ofNotifyEvent(touchEvents, touchEventArgs);
}

extern float winrtDPIx;
extern float winrtDPIy;

void ofAppWinRTWindow::OnPointerPressed(PointerEventArgs^ args, float dpi)
{
	bMousePressed = true;
	int button;
	if (args->CurrentPoint->Properties->IsLeftButtonPressed)
		button = OF_MOUSE_BUTTON_LEFT;
	else if (args->CurrentPoint->Properties->IsMiddleButtonPressed)
		button = OF_MOUSE_BUTTON_MIDDLE;
	else if (args->CurrentPoint->Properties->IsRightButtonPressed)
		button = OF_MOUSE_BUTTON_RIGHT;
	else
		return;
	ofEvents().notifyMousePressed(args->CurrentPoint->Position.X * dpi, args->CurrentPoint->Position.Y * dpi, button);

	int id;
	if (availableTouchIndices.empty())
	{
		id = currentTouchIndex++;
		touchInputTracker[args->CurrentPoint->PointerId] = id;
	}
	else
	{
		id = availableTouchIndices.front();
		availableTouchIndices.pop();
		touchInputTracker[args->CurrentPoint->PointerId] = id;
	}
	NotifyTouchEvent(id, ofEvents().touchDown, args, dpi);
}

void ofAppWinRTWindow::OnPointerReleased(PointerEventArgs^ args, float dpi)
{
	int button;
	if (args->CurrentPoint->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::LeftButtonReleased)
		button = OF_MOUSE_BUTTON_LEFT;
	else if (args->CurrentPoint->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased)
		button = OF_MOUSE_BUTTON_MIDDLE;
	else if (args->CurrentPoint->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::RightButtonReleased)
		button = OF_MOUSE_BUTTON_RIGHT;
	else
		return;
	ofEvents().notifyMouseReleased(args->CurrentPoint->Position.X * dpi, args->CurrentPoint->Position.Y * dpi, button);
	bMousePressed = false;

	int id = touchInputTracker[args->CurrentPoint->PointerId];
	availableTouchIndices.push(id); 
	touchInputTracker.erase(args->CurrentPoint->PointerId);
	NotifyTouchEvent(id, ofEvents().touchUp, args, dpi);
}

void ofAppWinRTWindow::OnPointerMoved(PointerEventArgs^ args, float dpi)
{
	double x = args->CurrentPoint->Position.X * dpi;
	double y = args->CurrentPoint->Position.Y * dpi;
	rotateMouseXY(ofGetOrientation(), x, y);
	if (bMousePressed)
		ofEvents().notifyMouseDragged(x, y, mouseInUse);
	else
		ofEvents().notifyMouseMoved(x, y);

	NotifyTouchEvent(touchInputTracker[args->CurrentPoint->PointerId], ofEvents().touchMoved, args, dpi);
}

static int TranslateWinrtKey(KeyEventArgs^ args)
{
	int key;
	VirtualKey virtualKey = args->VirtualKey;

	switch (virtualKey) {
	case VirtualKey::Escape:
		key = OF_KEY_ESC;
		break;
	case VirtualKey::F1:
		key = OF_KEY_F1;
		break;
	case VirtualKey::F2:
		key = OF_KEY_F2;
		break;
	case VirtualKey::F3:
		key = OF_KEY_F3;
		break;
	case VirtualKey::F4:
		key = OF_KEY_F4;
		break;
	case VirtualKey::F5:
		key = OF_KEY_F5;
		break;
	case VirtualKey::F6:
		key = OF_KEY_F6;
		break;
	case VirtualKey::F7:
		key = OF_KEY_F7;
		break;
	case VirtualKey::F8:
		key = OF_KEY_F8;
		break;
	case VirtualKey::F9:
		key = OF_KEY_F9;
		break;
	case VirtualKey::F10:
		key = OF_KEY_F10;
		break;
	case VirtualKey::F11:
		key = OF_KEY_F11;
		break;
	case VirtualKey::F12:
		key = OF_KEY_F12;
		break;
	case VirtualKey::Left:
		key = OF_KEY_LEFT;
		break;
	case VirtualKey::Right:
		key = OF_KEY_RIGHT;
		break;
	case VirtualKey::Up:
		key = OF_KEY_UP;
		break;
	case VirtualKey::Down:
		key = OF_KEY_DOWN;
		break;
	case VirtualKey::PageUp:
		key = OF_KEY_PAGE_UP;
		break;
	case VirtualKey::PageDown:
		key = OF_KEY_PAGE_DOWN;
		break;
	case VirtualKey::Home:
		key = OF_KEY_HOME;
		break;
	case VirtualKey::End:
		key = OF_KEY_END;
		break;
	case VirtualKey::Insert:
		key = OF_KEY_INSERT;
		break;
	case VirtualKey::LeftShift:
		key = OF_KEY_LEFT_SHIFT;
		break;
	case VirtualKey::LeftControl:
		key = OF_KEY_LEFT_CONTROL;
		break;
		//case VirtualKey::LeftAlt:
		//	key = OF_KEY_LEFT_ALT;
		//	break;
	case VirtualKey::LeftWindows:
		key = OF_KEY_LEFT_SUPER;
		break;
	case VirtualKey::RightShift:
		key = OF_KEY_RIGHT_SHIFT;
		break;
	case VirtualKey::RightControl:
		key = OF_KEY_RIGHT_CONTROL;
		break;
		//case GLFW_KEY_RIGHT_ALT:
		//	key = OF_KEY_RIGHT_ALT;
		//	break;
	case VirtualKey::RightWindows:
		key = OF_KEY_RIGHT_SUPER;
		break;
	case VirtualKey::Back:
		key = OF_KEY_BACKSPACE;
		break;
	case VirtualKey::Delete:
		key = OF_KEY_DEL;
		break;
	case VirtualKey::Enter:
		key = OF_KEY_RETURN;
		break;
	case VirtualKey::Tab:
		key = OF_KEY_TAB;
		break;
	case VirtualKey::Shift:
		key = OF_KEY_SHIFT;
		break;
	case (VirtualKey) 186:
		key = ';';
		break;
	case (VirtualKey) 192:
		key = '`';
		break;
	case (VirtualKey) 187:
		key = '=';
		break;
	case (VirtualKey) 222:
		key = '\'';
		break;
	case (VirtualKey) 188:
		key = ',';
		break;
	case (VirtualKey) 190:
		key = '.';
		break;
	case (VirtualKey) 191:
		key = '/';
		break;
	case (VirtualKey) 219:
		key = '[';
		break;
	case (VirtualKey) 221:
		key = ']';
		break;
	case (VirtualKey) 220:
		key = '\\';
		break;
	default:
		key = (int) args->VirtualKey;
		break;
	}

	//handle the special capital cases
	if (ofGetKeyPressed(OF_KEY_SHIFT))
	{
		switch (virtualKey)
		{
		case VirtualKey::Number0:
			key = ')';
			break;
		case VirtualKey::Number1:
			key = '!';
			break;
		case VirtualKey::Number2:
			key = '@';
			break;
		case VirtualKey::Number3:
			key = '#';
			break;
		case VirtualKey::Number4:
			key = '$';
			break;
		case VirtualKey::Number5:
			key = '%';
			break;
		case VirtualKey::Number6:
			key = '^';
			break;
		case VirtualKey::Number7:
			key = '&';
			break;
		case VirtualKey::Number8:
			key = '*';
			break;
		case VirtualKey::Number9:
			key = '(';
			break;
		case VirtualKey::Subtract:
			key = '_';
			break;
		case (VirtualKey) 186:
			key = ':';
			break;
		case (VirtualKey) 192:
			key = '~';
			break;
		case (VirtualKey) 187:
			key = '+';
			break;
		case (VirtualKey) 222:
			key = '"';
			break;
		case (VirtualKey) 188:
			key = '<';
			break;
		case (VirtualKey) 190:
			key = '>';
			break;
		case (VirtualKey) 191:
			key = '?';
			break;
		case (VirtualKey) 219:
			key = '{';
			break;
		case (VirtualKey) 221:
			key = '}';
			break;
		case (VirtualKey) 220:
			key = '|';
			break;
		}
	}

	//winrt spits out capital letters by default so convert keys to lower case if shift isn't held
	if (key >= 'A' && key <= 'Z' && !ofGetKeyPressed(OF_KEY_SHIFT))
		key += 'a' - 'A';

	return key;
}

void ofAppWinRTWindow::OnKeyPressed(KeyEventArgs^ args)
{
	events().notifyKeyPressed(TranslateWinrtKey(args));
}

void ofAppWinRTWindow::OnKeyReleased(KeyEventArgs^ args)
{
	events().notifyKeyReleased(TranslateWinrtKey( args));
}

void ofAppWinRTWindow::OnWindowSizeChanged(int width,int height){
	setWindowRect({0.0f,0.0f,(float)width,(float)height});
}

