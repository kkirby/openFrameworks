#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "InputEvent.h"

#include <mutex>
#include <queue>
#include <memory>

namespace AngleApp
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Renderer
	{
	public:
        Renderer(const std::shared_ptr<AngleApp::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(AngleApp::StepTimer const& timer);
		void Render();
		void AddPointerEvent(PointerEventType type, Windows::UI::Core::PointerEventArgs^ args);
		void AddKeyboardEvent(KeyboardEventType type, Windows::UI::Core::KeyEventArgs^ args);

		void OnPointerPressed(Windows::UI::Core::PointerEventArgs^ e);
        void OnPointerMoved(Windows::UI::Core::PointerEventArgs^ e);
        void OnPointerReleased(Windows::UI::Core::PointerEventArgs^ e);

		void OnKeyPressed(Windows::UI::Core::KeyEventArgs^ e);
		void OnKeyReleased(Windows::UI::Core::KeyEventArgs^ e);

	private:
       
        // Cached pointer to device resources.
        std::shared_ptr<AngleApp::DeviceResources> m_deviceResources;

        void ProcessEvents();
        
        // Variables used with the rendering loop.
        bool	m_loadingComplete;
        bool	m_setupComplete;

         std::queue<std::shared_ptr<InputEvent>> mInputEvents;
        std::mutex mMutex;
	};
}

