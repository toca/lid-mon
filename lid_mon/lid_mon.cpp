#include <Windows.h>
#include <cstdio>
#include <system_error>
#include <chrono>
#include <iomanip>



LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
HWND window;
std::string command_opened;
std::string command_closed;
bool firsttime = false;
void Opened();
void Closed();

// m a i n 
int main(int argc, char** argv)
{
	try {
		if (3 <= argc) {
			printf("Press 'Ctrl - C' to stop.\n");
			command_opened = argv[1];
			command_closed = argv[2];
		}
		else {
			printf("This tool detects the opening and closing of a laptop PC lid and executes any command.\n");
			printf("lid-mon <command_on_opend> <command_on_closed>");
			return 0;
		}


		// break (ctrl-c)
		::SetConsoleCtrlHandler([](DWORD event) -> BOOL {
			if (event == CTRL_C_EVENT)
			{
				LRESULT messageRes = ::SendMessageW(window, WM_DESTROY, 0, 0);

				printf("Break.\n");
				return TRUE;
			}
			return FALSE;
			}, TRUE);


		//register window    
		const wchar_t *WindowClassName = L"detect_lid_event_window";
		WNDCLASSEXW windowClass{};

		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.lpfnWndProc = windowProc;
		windowClass.lpszClassName = WindowClassName;
		ATOM registerResult = ::RegisterClassExW(&windowClass);
		if (!registerResult)
		{
			auto lastError = ::GetLastError();
			throw std::system_error(std::error_code((int)lastError, std::system_category()));
		}

		// create window
		HMODULE instance = ::GetModuleHandleW(L"");
		window = ::CreateWindowExW(
			0,
			WindowClassName,
			L"lid_mon_window",
			0,
			0, 0,
			0, 0,
			nullptr,
			nullptr,
			instance,
			nullptr
		);
		
		 
		// power
		HANDLE notificationHandle = ::RegisterPowerSettingNotification(window, &GUID_LIDSWITCH_STATE_CHANGE, DEVICE_NOTIFY_WINDOW_HANDLE);



		// main loop ----
		MSG message{};
		while (true)
		{
			BOOL messageRes = ::GetMessageW(&message, window, 0, 0);
			if (!messageRes)
			{
				break;
			}
			::TranslateMessage(&message);
			::DispatchMessageW(&message);
		}

		// clean up
		::UnregisterPowerSettingNotification(notificationHandle);




	} catch (std::exception e) {
		fprintf(stderr, "%s\n", e.what());
		return 1;
	}
	return 0;
}



LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_POWERBROADCAST:
		if (wParam != PBT_POWERSETTINGCHANGE) break;
		POWERBROADCAST_SETTING* setting = reinterpret_cast<POWERBROADCAST_SETTING*>(lParam);
		if(setting->PowerSetting != GUID_LIDSWITCH_STATE_CHANGE) break;
		if (*(setting->Data) == 0x00) {
			Closed();
		}
		else {
			Opened();
		}
		break;
	}
	return ::DefWindowProcW(window, message, wParam, lParam);
}

std::string dateString() {
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	struct tm tm;
	localtime_s(&tm, &t);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S%z", &tm);
	return std::string(buffer);

}

void Opened() {
	printf("opened: %s\n", dateString().c_str());

	if (command_opened.empty()) return;
	if (firsttime) {
		firsttime = false;
		return;
	}
	::ShellExecuteA(nullptr, "open", command_opened.c_str(), nullptr, nullptr, SW_SHOW);
}

void Closed() {
	printf("closed: %s\n", dateString().c_str());

	if (command_closed.empty()) return;
	if (firsttime) {
		firsttime = false;
		return;
	}
	::ShellExecuteA(nullptr, "open", command_closed.c_str(), nullptr, nullptr, SW_SHOW);
}

