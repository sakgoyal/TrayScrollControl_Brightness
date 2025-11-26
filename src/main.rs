#![allow(nonstandard_style)]

use windows_sys::{
	Win32::{
		Foundation::*,
		System::LibraryLoader::GetModuleHandleW,
		UI::{Shell::*, WindowsAndMessaging::*},
        UI::Input::*,
        Graphics::Gdi::PtInRect,
	},
	core::*,
};

const WM_TRAYICON: u32 = WM_USER + 1;
static mut NID: NOTIFYICONDATAW = unsafe { core::mem::zeroed() };

fn main() {
	let class_name = w!("TRAYICONSCROLL");

    let hInstance = unsafe { GetModuleHandleW(std::ptr::null()) };

    let wndclass = WNDCLASSW {
        lpfnWndProc: Some(WndProc),
        hInstance: hInstance,
        lpszClassName: class_name,
        ..Default::default()
    };


	let result = unsafe { RegisterClassW(&wndclass) };
    assert!(result != 0, "Failed to register window class: {:?}", unsafe { GetLastError() });

	let hwnd: HWND = unsafe { CreateWindowExW(0, class_name, std::ptr::null_mut(), 0, 0, 0, 0, 0, std::ptr::null_mut(), std::ptr::null_mut(), hInstance, std::ptr::null_mut()) };
    assert!(hwnd != std::ptr::null_mut(), "Failed to create window: {:?}", unsafe { GetLastError() });

	let icon_handle = unsafe { LoadIconW(std::ptr::null_mut(), IDI_APPLICATION) };
    assert!(!icon_handle.is_null(), "Failed to load icon: {:?}", unsafe { GetLastError() });

	unsafe {
		NID.cbSize = std::mem::size_of::<NOTIFYICONDATAW>() as u32;
		NID.uID = 1;
		NID.hWnd = hwnd;
		NID.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		NID.uCallbackMessage = WM_TRAYICON;
		NID.hIcon = icon_handle;
		NID.szTip = convert_string_to_wide("My Application Tooltip");
	}

	let result = unsafe { Shell_NotifyIconW(NIM_ADD, &raw const NID) };
	if result == 0 {
		panic!("Failed to add notification icon: {:?}", unsafe { GetLastError() });
	}

	let mut msg = MSG::default();
	unsafe {
		while GetMessageW(&mut msg, hwnd, 0, 0) != 0 {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	std::process::exit(msg.wParam as i32);
}

fn convert_string_to_wide(s: &'static str) -> [u16; 128] {
	let mut wide: [u16; 128] = [0; 128];
	let encoded: Vec<u16> = s.encode_utf16().collect();
	for (i, &c) in encoded.iter().enumerate() {
		wide[i] = c;
	}
	// ensure null termination
	if encoded.len() < wide.len() {
		wide[encoded.len()] = 0;
	}
	return wide;
}

unsafe extern "system" fn WndProc(hWnd: HWND, message: u32, wParam: WPARAM, lParam: LPARAM) -> LRESULT {
	CheckIfCursorIsInTrayIconBounds(hWnd);

    return unsafe { DefWindowProcW(hWnd, message, wParam, lParam) };

	// match message {
	// 	WM_INPUT => {
	// 		// Handle raw input for mouse wheel
    //         let mut dwSize: u32 = std::mem::size_of::<RAWINPUT>() as u32;
    //         let mut raw = RAWINPUT::default();
	// 	}
	// 	WM_TRAYICON => {
	// 		if lParam == WM_RBUTTONUP.try_into().unwrap() {
	// 			unsafe { ShowTrayMenu(hWnd) };
	// 		}
	// 	}
	// 	WM_DESTROY => {
	// 		unsafe { Shell_NotifyIconW(NIM_DELETE, &raw const NID) };
	// 		unsafe { PostQuitMessage(0) };
	// 	}
	// 	_ => {
	// 		unsafe { DefWindowProcW(hWnd, message, wParam, lParam) };
	// 	}
	// }
	// return 0;
}

fn CheckIfCursorIsInTrayIconBounds(hWnd: HWND) -> bool {
    return true;
    // let niid = NOTIFYICONIDENTIFIER {
    //     cbSize: std::mem::size_of::<NOTIFYICONIDENTIFIER>() as u32,
    //     hWnd: unsafe { NID.hWnd },
    //     uID: unsafe { NID.uID },
    //     guidItem: unsafe { core::mem::zeroed() },
    // };

    // let mut ptCursor: POINT = POINT::default();
    // let mut rcIcon: RECT = RECT::default();
    // if unsafe { GetCursorPos(&mut ptCursor) } != 0 &&
    //    unsafe { Shell_NotifyIconGetRect(&niid, &mut rcIcon) != 0 } &&
    //    unsafe { PtInRect(&rcIcon, ptCursor) } != 0 {
    //     print!("Cursor is within tray icon bounds.\n");
    //     // Cursor is in tray icon bounds
    //     // Register for raw input if not already registered
    //     return true;
    // } else {
    //     // Cursor is outside tray icon bounds
    //     // Unregister raw input if registered
    //     print!("Cursor is outside tray icon bounds.\n");
    //     return false;
    // }

}


unsafe fn ShowTrayMenu(hWnd: HWND) {
	let mut pt: POINT = POINT::default();
	unsafe { GetCursorPos(&mut pt) };

	let hMenu = unsafe { CreatePopupMenu() };
	unsafe { AppendMenuW(hMenu, MF_STRING, 1, "Exit".encode_utf16().collect::<Vec<u16>>().as_ptr()) };

	unsafe { SetForegroundWindow(hWnd) }; // Required for menu to disappear correctly

	let cmd = unsafe { TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, std::ptr::null()) };
	unsafe { DestroyMenu(hMenu) };

	if cmd == 1 {
		unsafe { PostMessageW(hWnd, WM_CLOSE, 0, 0) };
	}
}
