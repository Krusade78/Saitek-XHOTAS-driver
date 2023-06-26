//===========================================================================
//
// CPLSVR1 property pages implementation.
//
// Functions:
//  X36PROP_Page1_DlgProc()
//  InitDInput()
//
//===========================================================================

#include "ajd_cpl.h"
#include <prsht.h>
#include "pages.h"


//===========================================================================
// X36PROP_Page1_DlgProc
//
// Callback proceedure CPLSVR1 property page #1 (General).
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
INT_PTR CALLBACK X36PROP_Page1_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDIGameCntrlPropSheet_X *pdiCpl = (CDIGameCntrlPropSheet_X*)GetWindowLongPtr(hWnd, DWLP_USER);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // perform any required initialization... 
            // get ptr to our object
            pdiCpl = (CDIGameCntrlPropSheet_X*)((PROPSHEETPAGE *)lParam)->lParam;

            // Save our pointer so we can access it later
            SetWindowLongPtr(hWnd, DWLP_USER, (LPARAM)pdiCpl);
		}
        break;
	case WM_CLOSE:
		break;
    }

    return FALSE;
} //*** end X36PROP_Page1_DlgProc()
