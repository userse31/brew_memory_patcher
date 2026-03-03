#include "AEEModGen.h"          // Module interface definitions.
#include "AEEAppGen.h"          // Applet interface definitions.
#include "AEEShell.h"           // Shell interface definitions. 
#include "AEEStdLib.h"			// Standard C-esque includes.
#include "AEEHtmlViewer.h"		// God GUI stuff on BREW is such a pain in the fucking ass.

#include "CBrew_Memory_Patcher.h"
#include "Brew_Memory_Patcher_res.h"


/*-----------------------------------------------------------------------------
Applet Structure - Definition of the Applet Structure that's passed to Brew MP 
API functions. All variables in here are referenced via the applet structure 
pointer "pMe->", and will be able to be referenced as static.
-----------------------------------------------------------------------------*/
typedef struct _Brew_Memory_Patcher {
    AEEApplet  applet;    // First element of this structure must be AEEApplet.
    IDisplay * piDisplay; // Copy of IDisplay Interface pointer for easy access.
    IShell   * piShell;   // Copy of IShell Interface pointer for easy access.
    AEEDeviceInfo  deviceInfo; // Copy of device info for easy access.
    // Add your own variables here...
	IHtmlViewer *phtmlviewer;

} Brew_Memory_Patcher;

char *patch_buffer;
char *html_buffer;
Brew_Memory_Patcher * pYes;

/*-----------------------------------------------------------------------------
Function Prototypes
-----------------------------------------------------------------------------*/
static  boolean Brew_Memory_Patcher_HandleEvent(Brew_Memory_Patcher* pMe, 
                                                AEEEvent eCode,
                                                uint16 wParam, uint32 dwParam);
boolean Brew_Memory_Patcher_InitAppData(Brew_Memory_Patcher* pMe);
void    Brew_Memory_Patcher_FreeAppData(Brew_Memory_Patcher* pMe);
static void Brew_Memory_Patcher_DrawScreen(Brew_Memory_Patcher * pMe);
void viewer_callback(void* pvUser, HViewNotify* pNotify);
static void screen_finite_state_machine(int screen_id);
static void submit_manager(const char *url);
static void patch_memory();
static int chartohex(char x);

/*-----------------------------------------------------------------------------
Function Definitions
-----------------------------------------------------------------------------*/

/*=============================================================================
FUNCTION: AEEClsCreateInstance

DESCRIPTION:
    This function is invoked while the app is being loaded. All modules must 
    provide this function. Ensure to retain the same name and parameters for 
    this function. In here, the module must verify the ClassID and then invoke 
    the AEEApplet_New() function that has been provided in AEEAppGen.c. 

    After invoking AEEApplet_New(), this function can do app-specific 
    initialization. In this example, a generic structure is provided so that 
    app developers need not change the app-specific initialization section
    every time, except for a call to IDisplay_InitAppData().
    
    This is done as follows:
    InitAppData() is called to initialize the AppletData instance. It is the
    app developer's responsibility to fill in the app data initialization code
    of InitAppData(). The app developer is also responsible for releasing
    memory allocated for data contained in AppletData. This is done in 
    IDisplay_FreeAppData().

PROTOTYPE:
    int AEEClsCreateInstance(AEECLSID ClsId,
                             IShell * piShell, 
                             IModule * piModule,
                             void ** ppObj)

PARAMETERS:
    ClsId: [in]:
      Specifies the ClassID of the applet which is being loaded.

    piShell [in]:
      Contains pointer to the IShell object. 

    piModule [in]:
      Contains pointer to the IModule object of the current module to which this
      app belongs.

    ppObj [out]: 
      On return, *ppObj must point to a valid IApplet structure. Allocation of
      memory for this structure and initializing the base data members is done
      by AEEApplet_New().

DEPENDENCIES:
    None

RETURN VALUE:
    AEE_SUCCESS:
      If this app needs to be loaded and if AEEApplet_New()invocation was
      successful.
   
   AEE_EFAILED:
     If this app does not need to be loaded or if errors occurred in
     AEEApplet_New(). If this function returns FALSE, this app will not load.

SIDE EFFECTS:
    None
=============================================================================*/
int AEEClsCreateInstance(AEECLSID ClsId, IShell * piShell, IModule * piModule, 
						 void ** ppObj)
{
    *ppObj = NULL;

    // Confirm this applet is the one intended to be created (classID matches):
    if( AEECLSID_CBREW_MEMORY_PATCHER == ClsId ) {
        // Create the applet and make room for the applet structure.
        // NOTE: FreeAppData is called after EVT_APP_STOP is sent to
        //       HandleEvent.
	    if( TRUE == AEEApplet_New(sizeof(Brew_Memory_Patcher),
                        ClsId,
                        piShell,
                        piModule,
                        (IApplet**)ppObj,
                        (AEEHANDLER)Brew_Memory_Patcher_HandleEvent,
                        (PFNFREEAPPDATA)Brew_Memory_Patcher_FreeAppData) ) {
                     		
            // Initialize applet data. This is called before EVT_APP_START is
            // sent to the HandleEvent function.
		    if(TRUE == Brew_Memory_Patcher_InitAppData((Brew_Memory_Patcher*)*ppObj)) {
			    return AEE_SUCCESS; // Data initialized successfully.
		    }
		    else {
                // Release the applet. This will free the memory allocated for
                // the applet when AEEApplet_New was called.
                IApplet_Release((IApplet*)*ppObj);
                return AEE_EFAILED;
            }
        } // End AEEApplet_New
    }
    return AEE_EFAILED;
}


/*=============================================================================
FUNCTION: Brew_Memory_Patcher_InitAppData

DESCRIPTION:
    This function is called when the application is starting up, so the 
	initialization and resource allocation code is executed here.

PROTOTYPE:
    boolean Brew_Memory_Patcher_InitAppData(Brew_Memory_Patcher * pMe)

PARAMETERS:
    pMe [in]:
      Pointer to the AEEApplet structure. This structure contains information
      specific to this applet. It was initialized in AEEClsCreateInstance().
  
DEPENDENCIES:
    None

RETURN VALUE:
    TRUE:
      If there were no failures.

SIDE EFFECTS:
    None
=============================================================================*/
boolean Brew_Memory_Patcher_InitAppData(Brew_Memory_Patcher * pMe)
{
    // Save local copy for easy access:
	pYes=pMe;
    pMe->piDisplay = pMe->applet.m_pIDisplay;
    pMe->piShell   = pMe->applet.m_pIShell;

    // Get the device information for this handset.
    // Reference all the data by looking at the pMe->deviceInfo structure.
    // Check the API reference guide for all the handy device info you can get.
    
    
    
    // Insert your code here for initializing or allocating resources...

	patch_buffer=(char*)MALLOC(sizeof(char)*2048);
	html_buffer=(char*)MALLOC(sizeof(char)*2048);
	if(patch_buffer==NULL){
		return FALSE;//Cannot allocate memory.
	}
	if(html_buffer==NULL){
		return FALSE;//Cannot allocate memory.
	}
	ISHELL_GetDeviceInfo(pMe->applet.m_pIShell,&pMe->deviceInfo);
	pMe->deviceInfo.wStructSize = sizeof(pMe->deviceInfo);
	AEERect viewer_size;
	viewer_size.x=0;
	viewer_size.y=0;
	viewer_size.dx=pMe->deviceInfo.cxScreen;
	viewer_size.dy=pMe->deviceInfo.cyScreen;
	ISHELL_CreateInstance(pMe->piShell,AEECLSID_HTML,(void**)&pMe->phtmlviewer);
	IHTMLVIEWER_SetRect(pMe->phtmlviewer,&viewer_size);
	IHTMLVIEWER_SetActive(pMe->phtmlviewer,TRUE);
	IHTMLVIEWER_SetNotifyFn(pMe->phtmlviewer,viewer_callback,pMe);
	IHTMLVIEWER_SetProperties(pMe->phtmlviewer,HVP_SCROLLBAR);

    return TRUE;// No failures up to this point, so return success.
}



void Brew_Memory_Patcher_FreeAppData(Brew_Memory_Patcher * pMe)
{
    
}

static boolean Brew_Memory_Patcher_HandleEvent(Brew_Memory_Patcher* pMe, 
								AEEEvent eCode, uint16 wParam, uint32 dwParam)
{  
	if(pYes->phtmlviewer!=NULL){
		if(IHTMLVIEWER_HandleEvent(pYes->phtmlviewer,eCode,wParam,dwParam)){
			return TRUE;
		}
	}
    switch (eCode) {
        // Event to inform app to start, so start-up code is here:
        case EVT_APP_START:
            // Draw text on display screen.
			DBGPRINTF("CUNT");
            Brew_Memory_Patcher_DrawScreen(pMe);
            return TRUE;         

        // Event to inform app to exit, so shut-down code is here:
        case EVT_APP_STOP:
      	    return TRUE;

        // Event to inform app to suspend, so suspend code is here:
        case EVT_APP_SUSPEND:
      	    return TRUE;

        // Event to inform app to resume, so resume code is here:
        case EVT_APP_RESUME:
            // Redraw text on display screen.
            Brew_Memory_Patcher_DrawScreen(pMe); 
      	    return TRUE;

        // An SMS message has arrived for this app. 
        // The Message is in the dwParam above as (char *).
        // sender simply uses this format "//BREW:ClassId:Message", 
        // example //BREW:0x00000001:Hello World
        case EVT_APP_MESSAGE:
      	    return TRUE;

        // A key was pressed:
        case EVT_KEY:
      	    return FALSE;

        // Clamshell has opened/closed
        // wParam = TRUE if open, FALSE if closed
        case EVT_FLIP:
            return TRUE;
      
	    // Clamtype device is closed and reexposed when opened, and LCD 
        // is blocked, or keys are locked by software. 
        // wParam = TRUE if keygaurd is on
        case EVT_KEYGUARD:
            return TRUE;

        // If event wasn't handled here, then break out:
        default:
            break;
    }
    return FALSE; // Event wasn't handled.
}


/*=============================================================================
FUNCTION: Brew_Memory_Patcher_DrawScreen

DESCRIPTION:
	Draw text in the middle of the display screen.

PROTOTYPE:
    static void Brew_Memory_Patcher_DrawScreen(Brew_Memory_Patcher * pMe)

PARAMETERS:
    pMe [in]:
      Pointer to the AEEApplet structure. This structure contains information
      specific to this applet. It was initialized in AEEClsCreateInstance().

DEPENDENCIES:
    None

RETURN VALUE:
    None

SIDE EFFECTS:
    None
=============================================================================*/
static void Brew_Memory_Patcher_DrawScreen(Brew_Memory_Patcher * pMe)
{
	screen_finite_state_machine(0);
}

void viewer_callback(void* pvUser, HViewNotify* pNotify){
	switch(pNotify->code){
		case HVN_NONE:
			DBGPRINTF("No event!");
			break;
		case HVN_DONE:
			DBGPRINTF("Done event!");
		
			IHTMLVIEWER_Redraw(pYes->phtmlviewer);
			break;
		case HVN_JUMP:
			DBGPRINTF("Jump event!");
			DBGPRINTF("pszURL: %s",pNotify->u.jump.pszURL);
			//screen_finite_state_machine(ATOI(pNotify->u.jump.pszURL));
			IHTMLVIEWER_Redraw(pYes->phtmlviewer);
			break;
		case HVN_SUBMIT:
			DBGPRINTF("Submit event!");
			submit_manager(pNotify->u.jump.pszURL);
			break;
		case HVN_FOCUS:
			DBGPRINTF("Focus Event!");
			break;
		case HVN_REDRAW_SCREEN:
			DBGPRINTF("Redraw event!");
			break;
		case HVN_FULLSCREEN_EDIT:
			DBGPRINTF("Fullscreen edit event!");
			break;
		case HVN_INVALIDATE:
			DBGPRINTF("Invalid date event!");
			break;
		case HVN_PAGEDONE:
			DBGPRINTF("Page done event!");
			break;
		case HVN_CONTENTDONE:
			DBGPRINTF("Content done event!");
			break;
		default:
			DBGPRINTF("Not documented!");
	}
}

static void screen_finite_state_machine(int screen_id){
	for(int i=0;i<2048;i++){
		html_buffer[i]=0;
	}
	SNPRINTF(html_buffer,2048,"<b>Patch Memory</b><form action=\"a:b\"><input name=\"c\"></input><input type=\"submit\" value=\"Patch\"></input></form>");
	IHTMLVIEWER_SetData(pYes->phtmlviewer,html_buffer,-1);
	IHTMLVIEWER_Redraw(pYes->phtmlviewer);
}

static int chartohex(char x){
	switch(x){
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		case 'a':
		case 'A':
			return 10;
		case 'b':
		case 'B':
			return 11;
		case 'c':
		case 'C':
			return 12;
		case 'd':
		case 'D':
			return 13;
		case 'e':
		case 'E':
			return 14;
		case 'f':
		case 'F':
			return 15;
		default:
			return 0;
	}
}

static void patch_memory(){
	DBGPRINTF("%s",patch_buffer);
	bool patching=TRUE;
	int cur_token=0;
	unsigned char *patch_point;
	unsigned char write_value;
	unsigned int tmp;
	while(patching){
		//Find next token.
		for(int i=cur_token;i<2048;i++){
			if(patch_buffer[i]==0){
				return;
			}
			if(patch_buffer[i]=='.'){
				cur_token=i;
				break;
			}
		}
		//Get the values.
		tmp=chartohex(patch_buffer[cur_token+1]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+2]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+3]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+4]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+5]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+6]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+7]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+8]);
		patch_point=(unsigned char*)tmp;
		tmp=chartohex(patch_buffer[cur_token+10]);
		tmp=(tmp<<4)|chartohex(patch_buffer[cur_token+11]);
		write_value=(unsigned char)tmp;
		DBGPRINTF("patch_point: %p",patch_point);
		DBGPRINTF("write: %08x",write_value);
#ifndef AEE_SIMULATOR
		patch_point[0]=write_value;
#endif
		cur_token++;
	}

}

static void submit_manager(const char *url){
	DBGPRINTF("Submit: %s",url);
	for(int i=0;i<2048;i++){
		patch_buffer[i]=0;
	}
	for(int i=0;i<2048;i++){
		if(url[i+6]!=0){
			patch_buffer[i]=url[i+6];
		}else{
			break;
		}
	}
	patch_memory();
}