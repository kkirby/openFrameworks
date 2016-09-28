#pragma once

#include "StepTimer.h"
#include "DeviceResources.h"
#include "Renderer.h"


// Renders Direct2D and 3D content on the screen.
namespace AngleApp
{
    class InputEvent;

    class AngleAppMain : public AngleApp::IDeviceNotify
	{
	public:
        AngleAppMain(const std::shared_ptr<AngleApp::DeviceResources>& deviceResources);
		~AngleAppMain();
		void CreateWindowSizeDependentResources();
        void OnPointerPressed(Windows::UI::Core::PointerEventArgs^ e);
        void OnPointerMoved(Windows::UI::Core::PointerEventArgs^ e);
        void OnPointerReleased(Windows::UI::Core::PointerEventArgs^ e);

		void OnKeyPressed(Windows::UI::Core::KeyEventArgs^ e);
		void OnKeyReleased( Windows::UI::Core::KeyEventArgs^ e);


        void StopTracking() { m_tracking = false; }
        bool IsTracking() { return m_tracking; }
        void StartRenderLoop();
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		void Update();
		bool Render();

        void ofNotifyAppResume(int state = 0);
        void ofNotifyAppSuspend(int state = 0);

		// Cached pointer to device resources.
        std::shared_ptr<AngleApp::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<Renderer> m_renderer;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		AngleApp::StepTimer m_timer;

		// Track current input pointer position.
        bool	m_tracking;

	};
}