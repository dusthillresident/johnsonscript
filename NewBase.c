#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
//#include "mylib.c"

#include <X11/extensions/Xdbe.h>


#ifndef GUI_DEBUG
 #define GUI_DEBUG 0
#endif
#define CopyPasteDebug 0


//#define NewBase_HaventRemovedThisYet


// =======================================================================================
// =======================================================================================
// =======================================================================================

Display *Mydisplay;
Window  Mywindow;
XEvent  Myevent;
int     Myscreen;
GC	MyGC;
Drawable Mydrawable;
XdbeBackBuffer Mybackbuffer;
int newbase_manual_refresh_mode=0;
int WinW,WinH;
int SDepth;
int NewBase_last_fg_col=0;
int NewBase_last_bg_col=0;
//char *BBCFont[256];
char **BBCFont = NULL;
// declare key buffer here
#define KeyBuffSize 256
int KeyboardBuffer[KeyBuffSize];
int KeyboardBufferReadPos=0;
int KeyboardBufferWritePos=0;
char *iskeypressed = NULL;
Atom WmDeleteWindowAtom;

volatile int mouse_x=0,mouse_y=0,mouse_z=0,mouse_b=0;
#ifdef NewBase_HaventRemovedThisYet
volatile int xflush_for_every_draw=0;
#endif
volatile int newbase_is_running=0;
volatile int exposed=0;
volatile int wmclosed=0;
volatile int wmcloseaction=0;


void DefaultFont();

void Wait(int w);


#define MyInit(w,h) (NewBase_MyInit(w,h,0))
#define MyInit_Threading(w,h) (NewBase_MyInit(w,h,1))


// ===========================================================================================================
// ===========================================================================================================
// ================ HORRIBLE GARBAGE FOR MANAGING CLIPBOARD ==================================================
// ===========================================================================================================
// ===========================================================================================================


volatile char *PasteBuffer = NULL; volatile size_t PasteBufferContentsSize = 0; // this buffer holds the contents of the clipboard we receive from others
//volatile Atom PasteBufferContentsAtom;
volatile int CopyPasteOngoing = 0; // 1 == paste requested, 2 == incoming incr transfer, 3 == outgoing incr transfer
#define CPO__PASTE_CONVERSION_REQUESTED 1
#define CPO__INCOMING_INCR 2
#define CPO__OUTGOING_INCR 3
volatile int CopyPasteRequested = 0; // 0 == no request right now, 1 == paste requested, 2 == copy requested
#define CPR__PASTE_REQUESTED 1
#define CPR__COPY_REQUESTED 2
volatile int PasteSucceeded = 0;
volatile Atom PasteRequestTypeAtom;
char *CopyBuffer = NULL; size_t CopyBufferContentsSize = 0; Atom CopyBufferContentsAtom; // this buffer holds the contents of the clipboard when we are the owner of the CLIPBOARD selection
volatile char *NewCopyBuffer; volatile size_t NewCopyBufferContentsSize = 0; volatile Atom NewCopyBufferContentsAtom; // in order to Copy, we load stuff into this buffer, then ask the NewBase thread to put it onto the clipboard by setting CopyPasteRequested to CPR__COPY_REQUESTED
Window CopyIncrRequestorWindow;
Atom   CopyIncrPropertyAtom;
unsigned int    CopyIncrOffset = 0;
int CopyIncrChunkSize = 0;
//Time CopyPasteLastEventTime;
#define COPY_PASTE_INCR_TIMEOUT_LOOPCOUNT 400
#define COPY_PASTE_CONVERSIONREQUEST_TIMEOUT_LOOPCOUNT 400
int CopyPasteTimeoutCounter = 0;
Atom CopyIncrTargetAtom;
Atom MyPasteAtom;
Atom MyClipboardAtom;
Atom MyTargetsAtom;
Atom MyIncrAtom;
Atom MyStringAtom;
Atom MyUtf8Atom;
Atom MyBmpAtom;

// internal function
int NB_ItemsizeConversion(int actual_format){
 int item_size;
 switch(actual_format){
  case 8:  item_size = sizeof(char); break;
  case 16: item_size = sizeof(short); break;
  case 32: item_size = sizeof(long); break;
  default:
  #if CopyPasteDebug
   printf("CopyPasteDebug: NB_ItemsizeConversion: bad actual_format\n"); tb(); exit(0);
  #endif
   return 0;
 }
 return item_size;
}

// internal function
void NB_CopyPasteRequestHandler(){
 #if CopyPasteDebug
 printf("CopyPasteDebug: entering NB_CopyPasteRequestHandler()\n");
 #endif
 switch( CopyPasteRequested ){
  case CPR__PASTE_REQUESTED: {
   #if CopyPasteDebug
   printf("CopyPasteDebug: Handling Paste request\n");
   #endif
   Window owner = XGetSelectionOwner(Mydisplay, MyClipboardAtom);
   if( owner == Mywindow ){ // we own the clipboard, that means we already have the clipboard contents in CopyBuffer
    if( PasteRequestTypeAtom == CopyBufferContentsAtom ){
     if(PasteBuffer){ // when there's a paste request, we can be sure the other thread is not accessing PasteBuffer, because it is waiting for the paste to complete
      free((void*)PasteBuffer);
     }
     #if CopyPasteDebug
     if( ! CopyBuffer ){
      tb();
      printf("CopyPasteDebug: paste requested and we own the clipboard but CopyBuffer is NULL, this should never happen\n");
      exit(0);
     }
     #endif
     PasteBuffer = malloc(CopyBufferContentsSize);
     if(! PasteBuffer){
      printf("malloc failed during Paste\n");
      exit(0);
     }
     memcpy((void*)PasteBuffer,(void*)CopyBuffer,CopyBufferContentsSize);
     PasteBufferContentsSize = CopyBufferContentsSize;
     //---
     PasteSucceeded = 1;
     CopyPasteOngoing   = 0;
     CopyPasteRequested = 0;
    }else{
     PasteSucceeded = 0;
     CopyPasteOngoing   = 0;
     CopyPasteRequested = 0;
    }
    return;
   }//endif we already own the clipboard
   if( owner == None ){ // if nobody owns the clipboard
    PasteSucceeded = 0;
    CopyPasteOngoing   = 0;
    CopyPasteRequested = 0;
    return;
   }//endif nobody owns the clipboard, that is, clipboard is empty & nothing to paste
   // if we got this far, it means the clipboard has something on it. let's request conversion of the clipboard to the sort of data we want
   CopyPasteOngoing = CPO__PASTE_CONVERSION_REQUESTED;
   CopyPasteTimeoutCounter = COPY_PASTE_CONVERSIONREQUEST_TIMEOUT_LOOPCOUNT;
   XConvertSelection( Mydisplay,
                      MyClipboardAtom,
                      PasteRequestTypeAtom,
                      MyPasteAtom,
                      Mywindow,
                      CurrentTime );
   // waiting for SelectionNotify
   return;
  } break;//end of paste request
  case CPR__COPY_REQUESTED: {
   #if CopyPasteDebug
   printf("CopyPasteDebug: Handling Copy request\n");
   #endif
   free( (void*) CopyBuffer );
   CopyBuffer = (char*)NewCopyBuffer;
   CopyBufferContentsSize = NewCopyBufferContentsSize;
   CopyBufferContentsAtom = NewCopyBufferContentsAtom;
   NewCopyBuffer = NULL; NewCopyBufferContentsSize=0;
   XSetSelectionOwner(Mydisplay, MyClipboardAtom, Mywindow, CurrentTime); 
   CopyPasteRequested = 0;
  } break;
 }//endcase
}//endproc

// internal function
void NB_SelectionNotifyHandler( XSelectionEvent *sev ){
 switch( CopyPasteOngoing ){
  default: {
   #if CopyPasteDebug 
   printf("CopyPasteDebug: NB_SelectionNotifyHandler: unhandled!\n");
   #endif
  } break;
  case CPO__PASTE_CONVERSION_REQUESTED: { // waiting for incoming data, after having requested conversion of the CLIPBOARD selection
   if( sev->property == None ){
    // conversion could not be performed. maybe the clipboard doesn't contain text, or there's some other problem, like the clipboard owner doesn't support the target we requested
    PasteSucceeded = 0;
    CopyPasteOngoing   = 0;
    CopyPasteRequested = 0;
    return;
   }else{
    Atom da, type;
    int actual_format_return;
    unsigned long nitems_return, bytes_after_return, datasize;
    unsigned char *prop_ret = NULL;
    /* PENISEXPERT: Dummy call to get type and size. */
    XGetWindowProperty( Mydisplay, 
                        Mywindow,
                        MyPasteAtom,
                        0, // long_offset
                        0, // long_length
                        False, // delete
                        AnyPropertyType, // req_type
                        &type, // actual_type_return
                        &actual_format_return,
                        &nitems_return,
                        &bytes_after_return, //bytes_after_return
                        &prop_ret ); // prop_return
    XFree(prop_ret);
    if( type != MyIncrAtom ){
     #if CopyPasteDebug 
     printf("CopyPasteDebug: Doing normal paste (not incr),");
     printf(" property size: %lu\n", bytes_after_return);
     #endif
     /* PENISEXPERT: Read the data in one go. */
     XGetWindowProperty(Mydisplay,
                        Mywindow,
                        MyPasteAtom,
                        0,			//long_offset
                        bytes_after_return,	//long_length
                        False,			//delete
                        AnyPropertyType,	//req_type
                        &da,			//actual_type_return
                        &actual_format_return,	//actual_format_return
                        &nitems_return,		//nitems_return
                        &bytes_after_return,	//bytes_after_return
                        &prop_ret);
     free( (void*)PasteBuffer ); // I checked that it's safe to pass NULL to free. "If ptr is NULL, no operation is performed."
     datasize = nitems_return * NB_ItemsizeConversion( actual_format_return ) ;
     PasteBufferContentsSize = datasize;
     PasteBuffer = calloc( 1, 1 + datasize );
     
     memcpy( (void*)PasteBuffer, prop_ret, datasize );
     //printf("%s", prop_ret); //fflush(stdout);
     XFree(prop_ret);

     /* PENISEXPERT Signal the selection owner that we have successfully read the
      *             data. */
     XDeleteProperty(Mydisplay, Mywindow, MyPasteAtom);

     PasteSucceeded = 1;
     CopyPasteOngoing   = 0;
     CopyPasteRequested = 0;
     return;
    }else{
     #if CopyPasteDebug 
     printf("CopyPasteDebug: Starting incr paste\n");
     #endif
     // ----
     free( (void*)PasteBuffer ); 
     PasteBuffer = NULL;
     PasteBufferContentsSize=0;
     //CopyPasteLastEventTime = sev->time;
     CopyPasteTimeoutCounter = COPY_PASTE_INCR_TIMEOUT_LOOPCOUNT;
     CopyPasteOngoing = CPO__INCOMING_INCR;
     XDeleteProperty(Mydisplay, Mywindow, MyPasteAtom); //delete the event to start the incr process
     return;
    }

   }
  } break; // end of 'waiting for incoming data' block
  case CPO__INCOMING_INCR: { // incr process. as far as I can tell, we shouldn't receive a SelectionNotify while this is ongoing and even if we do, just don't do anything with it
   #if CopyPasteDebug 
   printf("SelectionNotify during incr paste. This isn't supposed to happen\n");
   #endif
  } break;
 }//endcase
}//endproc

// internal function
void NB_PropertyNotifyHandler(){
 #if CopyPasteDebug 
 printf("NB_PropertyNotifyHandler\n");
 #endif
 switch( CopyPasteOngoing ){
  case CPO__INCOMING_INCR: {
   if( Myevent.xproperty.atom == MyPasteAtom && Myevent.xproperty.state == PropertyNewValue ){
    #if CopyPasteDebug 
    printf("CopyPasteDebug: NB_PropertyNotifyHandler: continuing incr Paste\n");
    #endif
    // update time for timeout check
    //CopyPasteLastEventTime = Myevent.xproperty.time; // I don't think I can find a way to make this work. I'll leave it here for now
    CopyPasteTimeoutCounter = COPY_PASTE_INCR_TIMEOUT_LOOPCOUNT; // instead I have to do this shit 
    // xgetproperty, check the size and format of the property.
    Atom actual_type_return;
    int actual_format_return;
    unsigned long nitems_return, bytes_after_return;
    unsigned char *prop_return;
    int item_size=0, datasize;
    XGetWindowProperty( Mydisplay,
                        Mywindow,
                        MyPasteAtom,
                        0,
			0,
			False,
			AnyPropertyType,
			&actual_type_return,
                        &actual_format_return,
                        &nitems_return,
                        &bytes_after_return,
                        &prop_return );
    XFree(prop_return); //this seems to necessarily happen after this first initial XGetWindowProperty
    // if property data size is 0, that means we have completed the incr transfer jesus christ yeah.
    #if CopyPasteDebug 
    printf("CopyPasteDebug: bytes_after_return == %lu\n",bytes_after_return); 
    #endif
    if( bytes_after_return == 0 ){
     #if CopyPasteDebug 
     printf("CopyPasteDebug: Completed incr paste transfer\n");
     #endif
     XDeleteProperty( Mydisplay, Mywindow, MyPasteAtom );
     PasteSucceeded = 1;
     CopyPasteOngoing   = 0;
     CopyPasteRequested = 0;
     return;
    }
    // if we get here, the property contains text. we obtained the size in bytes_after_return before.
    // now we get the chunk of data.
    XGetWindowProperty( Mydisplay,
                        Mywindow,
                        MyPasteAtom,
                        0,
			(long) bytes_after_return,
			False,
			AnyPropertyType,
			&actual_type_return,
                        &actual_format_return,
                        &nitems_return,
                        &bytes_after_return,
                        &prop_return );
    // then do this: allocate memory (or extra memory) for the data, and load it in
    item_size = NB_ItemsizeConversion(actual_format_return);
    datasize = nitems_return * item_size;
    
    if( !PasteBuffer ){
     PasteBuffer = malloc( datasize );
     if( ! PasteBuffer ){
      printf("malloc failed during Paste\n");
      exit(0);
     }
     //PasteBufferContentsSize = datasize;
    }else{
     PasteBuffer = realloc( (void*)PasteBuffer, PasteBufferContentsSize + datasize );
     if(! PasteBuffer ){
      printf("realloc failed during Paste\n");
      exit(0);
     }
    }
    memcpy( (void*)&PasteBuffer[PasteBufferContentsSize], prop_return, datasize );
    PasteBufferContentsSize += datasize;

    XFree( prop_return );
    XDeleteProperty( Mydisplay, Mywindow, MyPasteAtom );
    XFlush( Mydisplay );

    #if CopyPasteDebug 
    printf("CopyPasteDebug: received incr chunk\n");
    #endif

    return;
    
   }else{
    #if CopyPasteDebug 
    printf("CopyPasteDebug: ignoring irrelevent PropertyNotify during incoming incr transfer\n");
    #endif
   }//endif
  } break; // end of block: dealing with an incoming incr transfer
  case CPO__OUTGOING_INCR: {

   #if CopyPasteDebug 
   printf("CopyPasteDebug: NB_PropertyNotifyHandler: continuing incr Copy\n");
   #endif

   if( Myevent.xproperty.state != PropertyDelete ){
    #if CopyPasteDebug 
    printf("CopyPasteDebug: ignoring irrelevent PropertyNotify during outgoing incr transfer\n");
    #endif
    return;
   }
   // update time for timeout check
   //CopyPasteLastEventTime = Myevent.xproperty.time; // I can't make this work but I will leave it here in case I find a way later
   CopyPasteTimeoutCounter = COPY_PASTE_INCR_TIMEOUT_LOOPCOUNT; // sadly I have to do this shit
   // alright lets go.
   int thischunksize = CopyIncrChunkSize;
   if( (CopyIncrOffset + CopyIncrChunkSize) >= CopyBufferContentsSize ){
    thischunksize = (CopyBufferContentsSize % CopyIncrChunkSize);
   }else{
    thischunksize = CopyIncrChunkSize;
   }
   #if CopyPasteDebug 
   printf("CopyPasteDebug: thischunksize is %d\n", thischunksize);
   #endif
   if( CopyIncrOffset < CopyBufferContentsSize ){
    #if CopyPasteDebug
    printf("sending chunk\n");
    #endif
    XChangeProperty( Mydisplay,
                     CopyIncrRequestorWindow,
                     CopyIncrPropertyAtom,
                     CopyIncrTargetAtom,
                     8, PropModeReplace, &((unsigned char*)CopyBuffer)[CopyIncrOffset],
                     (int) thischunksize );
    CopyIncrOffset += thischunksize;
   }else{
    #if CopyPasteDebug 
    printf("finished outgoing incr transfer\n");
    #endif
    XChangeProperty(Mydisplay, CopyIncrRequestorWindow, CopyIncrPropertyAtom, CopyIncrTargetAtom, 8, PropModeReplace, 0, 0);
    //XSelectInput(Mydisplay, CopyIncrRequestorWindow, 0); // probably not necessary
    CopyPasteOngoing = 0;
   }
   XFlush(Mydisplay);
  } break; // end of block: doing an outgoing incr transfer
 }//endcase
}//endproc


// internal function
// returns 1 if successfully obtained data from clipboard, otherwise returns 0
int _NB_PasteData(Atom request_type){
 if(!newbase_is_running){
  return 0;
 }
 while( CopyPasteRequested ){
  Wait(1);
 }
 PasteRequestTypeAtom = request_type;
 CopyPasteRequested = CPR__PASTE_REQUESTED;
 #if CopyPasteDebug 
 int c=0;
 #endif
 while( CopyPasteRequested ){
  #if CopyPasteDebug 
  c++;
  #endif
  Wait(1);
 }
 #if CopyPasteDebug 
 printf("CopyPasteDebug: NB_PasteData wait count: %d\n",c);
 #endif
 return PasteSucceeded;
}

// intended to be used externally
int NB_PasteBmp(){
 return _NB_PasteData(MyBmpAtom);
}

// intended to be used externally
int NB_PasteText(){
 return _NB_PasteData(MyUtf8Atom) || _NB_PasteData(MyStringAtom);
}


// ==============================================================================
// === 'Copy', putting stuff ON the clipboard (sending data to other clients) ===
// ==============================================================================


// internal function
void NB_Copy_DenyRequest(XSelectionRequestEvent *sev){
 XSelectionEvent ssev;
 /* PENISEXPERT: All of these should match the values of the request. */
 ssev.type = SelectionNotify;
 ssev.requestor = sev->requestor;
 ssev.selection = sev->selection;
 ssev.target = sev->target;
 ssev.property = None;  /* PENISEXPERT: signifies "nope" */
 ssev.time = sev->time;
 XSendEvent(Mydisplay, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

// internal function
void NB_SelectionRequestHandler(){
 if( CopyPasteOngoing ){
  // only bother to deal with one at a time
  tb();
  NB_Copy_DenyRequest( &Myevent.xselectionrequest );
  return;
 }
 if( XGetSelectionOwner(Mydisplay, MyClipboardAtom) != Mywindow ){
  // we don't own the selection, I don't know why we are getting a SelectionRequest
  #if CopyPasteDebug 
  printf("CopyPasteDebug: got SelectionRequest but we don't own the selection\n");
  #endif
  NB_Copy_DenyRequest( &Myevent.xselectionrequest );
  return;
 }
 if( ! CopyBuffer ){
  // oh no!!!!! how did this happen?
  #if CopyPasteDebug 
  printf("CopyPasteDebug: SelectionRequest and we own the clipboard but the copy buffer is empty, should never happen, exiting\n");
  #endif
  exit(0);
 }

 // ----
 // ----

 Window requestor_window  = Myevent.xselectionrequest.requestor;
 Atom requestors_property = Myevent.xselectionrequest.property;
 Atom requested_target    = Myevent.xselectionrequest.target;
 unsigned int chunk_size = 0;

 if( requested_target == MyTargetsAtom ){

  #if CopyPasteDebug 
  printf("CopyPasteDebug: SelectionRequestHandler: doing TARGETS\n");
  #endif

  XSelectionRequestEvent *sev = (XSelectionRequestEvent*)&Myevent.xselectionrequest;
  Atom type_atoms[] = { MyTargetsAtom, CopyBufferContentsAtom };
  XChangeProperty( Mydisplay,
                   requestor_window,
                   requestors_property,
                   XA_ATOM,
                   32, PropModeReplace, (unsigned char*) type_atoms,
                   (int) (sizeof(type_atoms) / sizeof(Atom)) );
  XEvent response;
  response.xselection.type = SelectionNotify;
  response.xselection.property = requestors_property;
  response.xselection.display = sev->display;
  response.xselection.requestor = requestor_window;
  response.xselection.selection = sev->selection;
  response.xselection.target = sev->target;
  response.xselection.time = sev->time;
  XSendEvent(Mydisplay, requestor_window, 0, 0, &response);
  XFlush(Mydisplay);
  
 }else if( requested_target == CopyBufferContentsAtom || ( requested_target == MyUtf8Atom && CopyBufferContentsAtom == MyStringAtom ) ){

  #if CopyPasteDebug 
  printf("CopyPasteDebug: SelectionRequestHandler: doing paste\n");
  if( requested_target == MyUtf8Atom ){
   printf("CopyPasteDebug: 'UTF8_STRING' requested, pretending we have that instead of just 'STRING' and serving the request\n");
  }
  #endif

  CopyIncrTargetAtom = requested_target;

  unsigned int chunk_size = XExtendedMaxRequestSize(Mydisplay) / 4;
  if( ! chunk_size ){
   chunk_size = XMaxRequestSize(Mydisplay) / 4;
  }
  if( CopyBufferContentsSize > chunk_size ){

   #if CopyPasteDebug 
   printf("CopyPasteDebug: CopyBufferContentsSize large enough, starting incr copy (outgoing incr)\n");
   #endif

   CopyPasteOngoing = CPO__OUTGOING_INCR;

   CopyIncrRequestorWindow = requestor_window;
   CopyIncrPropertyAtom = requestors_property;

   // We need to know when this requesting window changes its properties
   XChangeProperty(Mydisplay, requestor_window, requestors_property, MyIncrAtom, 32, PropModeReplace, 0, 0);
   XSelectInput(Mydisplay, requestor_window, PropertyChangeMask);

   XSelectionRequestEvent *sev = (XSelectionRequestEvent*)&Myevent.xselectionrequest;
   XEvent response;
   response.xselection.type = SelectionNotify;
   response.xselection.property = requestors_property;
   response.xselection.display = sev->display;
   response.xselection.requestor = requestor_window;
   response.xselection.selection = sev->selection;
   response.xselection.target = sev->target;
   response.xselection.time = sev->time;
   XSendEvent(Mydisplay, requestor_window, 0, 0, &response);
   XFlush(Mydisplay);

   CopyIncrOffset = 0;    
   CopyIncrChunkSize = chunk_size;
   //CopyPasteLastEventTime = sev->time;
   CopyPasteTimeoutCounter = COPY_PASTE_INCR_TIMEOUT_LOOPCOUNT;

   return; 
 
  }else{
   #if CopyPasteDebug 
   printf("CopyPasteDebug: CopyBufferContentsSize small enough, sending data all at once\n");
   #endif
   XSelectionEvent ssev;
   XChangeProperty(Mydisplay, requestor_window, requestors_property, requested_target, 8, PropModeReplace,
                    (unsigned char *)CopyBuffer, CopyBufferContentsSize);
   ssev.type = SelectionNotify;
   ssev.requestor = Myevent.xselectionrequest.requestor;
   ssev.selection = Myevent.xselectionrequest.selection;
   ssev.target = Myevent.xselectionrequest.target;
   ssev.property = Myevent.xselectionrequest.property;
   ssev.time = Myevent.xselectionrequest.time;

   XSendEvent(Mydisplay, Myevent.xselectionrequest.requestor, True, NoEventMask, (XEvent *)&ssev);
   XFlush(Mydisplay);
  }

 }else{
  #if CopyPasteDebug 
  printf("CopyPasteDebug: ");
  char *an = XGetAtomName(Mydisplay, Myevent.xselectionrequest.target);
  printf("denying request of type '%s'\n", an);
  if (an) XFree(an);
  #endif
  NB_Copy_DenyRequest( &Myevent.xselectionrequest );
 }

 return;

}//endproc

//internal function
void _NB_CopyData(void *d, size_t n, Atom datatype){
 if( ! newbase_is_running ){
  return;
 }
 if( ! n ){
  return;
 }
 if( d == NULL ){
  printf("CopyPasteDebug: _NB_CopyData: given NULL pointer\n");
  return;
 }
 while(CopyPasteRequested){
  Wait(1);
 }
 // ----
 if(NewCopyBuffer){
  free( (void*) NewCopyBuffer );
 }
 NewCopyBuffer = malloc(n);
 if( ! NewCopyBuffer ){
  printf("malloc failed during Copy\n");
  exit(0);
 }
 memcpy((void*)NewCopyBuffer, d, n);
 NewCopyBufferContentsSize=n;
 NewCopyBufferContentsAtom = datatype;

 CopyPasteRequested = CPR__COPY_REQUESTED;
 return;
}

// intended to be used externally
void NB_CopyTextN(char *s, size_t n){
 _NB_CopyData((void*)s,n,MyStringAtom);
}
// intended to be used externally
void NB_CopyText(char *s){
 size_t size;
 size = strlen(s);
 NB_CopyTextN(s,size); 
}


// ===========================================================================================================
// ===========================================================================================================
// ================ END OF HORRIBLE GARBAGE FOR MANAGING CLIPBOARD ===========================================
// ===========================================================================================================
// ===========================================================================================================


void NewBase_MyInit(int winwidth,int winheight,int enablethreading){
 if(newbase_is_running) return;
 if(enablethreading){
  if( !XInitThreads() ){
   printf("XInitThreads failed\n");
   exit(1);
  }
 }
 Mydisplay=XOpenDisplay(NULL);
 // Open connection to server
 if(Mydisplay==NULL){
  printf("couldn't open display\n");
  exit(1);
 }
 Myscreen=DefaultScreen(Mydisplay);
 // Create window
 Mywindow=XCreateSimpleWindow(Mydisplay, RootWindow(Mydisplay, Myscreen), 10,10,winwidth,winheight, 1,
                            WhitePixel(Mydisplay,Myscreen),BlackPixel(Mydisplay,Myscreen) );
 // Process Window Close Event through event handler so XNextEvent does not fail
 // note: look up these functions to try to understand how/why this works
 Atom atom = XInternAtom( Mydisplay, "WM_DELETE_WINDOW", 0 );
 WmDeleteWindowAtom = atom;
 //printf("FUFFFU %d\n",atom);
 XSetWMProtocols(Mydisplay, Mywindow, &atom, 1);
 // prepare atom stuff for copy pasting
 MyPasteAtom = XInternAtom(Mydisplay, "PENISMAGIC", False);
 MyClipboardAtom = XInternAtom(Mydisplay, "CLIPBOARD", False);
 MyIncrAtom = XInternAtom(Mydisplay, "INCR", False);
 MyTargetsAtom = XInternAtom(Mydisplay, "TARGETS", False);
 MyStringAtom = XInternAtom(Mydisplay, "STRING", False);
 MyUtf8Atom = XInternAtom(Mydisplay, "UTF8_STRING", False);
 MyBmpAtom = XInternAtom(Mydisplay, "image/bmp", False);
 // Select the kind of events we're interested in 
 int xselinputreturnvalue;
 xselinputreturnvalue = XSelectInput(Mydisplay, Mywindow, ExposureMask | KeyPressMask |KeyReleaseMask| ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask | PropertyChangeMask );
 //printf("XSelectInput returns: %d\n",xselinputreturnvalue);
 // Map (show) the window
 XMapWindow(Mydisplay, Mywindow);
 // test
 MyGC = DefaultGC(Mydisplay,Myscreen);
 /*
 XFontStruct *xfontstruct = XLoadQueryFont(Mydisplay,"-*-*-medium-r-normal-*-17-120-100-100-*-0-*-*");
 XSetFont(Mydisplay, MyGC, xfontstruct->fid);
 */

 XSetForeground(Mydisplay,MyGC,WhitePixel(Mydisplay,Myscreen));
 XSetBackground(Mydisplay,MyGC,0x306030);

 //XDrawLine(Mydisplay, Mywindow, MyGC, 0,0, winwidth,winheight);

 XStoreName(Mydisplay,Mywindow, "Johnsonscript graphics");

 WinW=winwidth;
 WinH=winheight;

 //printf("whitepixel %d\n",WhitePixel(Mydisplay,Myscreen));
 SDepth=0;
 int i,wp;
 wp = WhitePixel(Mydisplay,Myscreen);
 for(i=0;i<31;i++){
  SDepth += (wp & 1);
  wp = wp >> 1;
 }
 //printf("SDepth %d\n",SDepth);
 
 if(BBCFont == NULL){
  BBCFont = calloc(256,sizeof(void*));
  char *c=calloc(8*256,sizeof(char));
  for(i=0;i<256;i++){
   BBCFont[i]=c+8*i;
  }
 }
 DefaultFont();

 // XdbeUndefined, XdbeBackground, XdbeUntouched, or XdbeCopied
 #define LetsTryChangingThisAround  XdbeBackground
 // 'XdbeBackground' seemed to cause the least GPU usage according to nvidia x server settings tool
 Mydrawable = Mywindow;
 Mybackbuffer = XdbeAllocateBackBufferName(Mydisplay, Mywindow, LetsTryChangingThisAround);
/*
 #ifndef TimeConflictBullshit
 XRANDrand=(unsigned long int) time(NULL);
 XRANDranb=(unsigned long int) ~time(NULL);
 #endif
*/
 #if 1
 XSetWindowBackgroundPixmap(Mydisplay, Mywindow, None);
 #endif

 #if 0
 XSetWindowAttributes bullshit;
 bullshit.win_gravity = StaticGravity;
 XChangeWindowAttributes(Mydisplay, Mywindow, CWWinGravity, &bullshit);
 #endif

 if(iskeypressed == NULL){
  iskeypressed = calloc(256,sizeof(char));
 }else{
  int i;
  for(i=0;i<256;i++){
   iskeypressed[i]=0;
  }
 }

 void ClearKeyboardBuffer();
 ClearKeyboardBuffer();
 /*
 void Gcol(int r, int g, int b);
 void GcolBG(int r, int g, int b);
 GcolBG(0,0,0);
 Gcol(255,255,255);
 */
 do{
  //tb();
  XNextEvent(Mydisplay, &Myevent);
 }while( ! (Myevent.type==MapNotify) );
 newbase_is_running=1;
}

void SetWindowTitle(char *title){
 if(!newbase_is_running)return;
 XStoreName(Mydisplay,Mywindow,title);
}

void MyCleanup(){
#ifdef NewBase_HaventRemovedThisYet
 xflush_for_every_draw=0;
#endif
 Wait(4);
 void RefreshOn();
 RefreshOn();
 Wait(1);
 newbase_is_running=0;
 XKeyEvent thisrubishevent; thisrubishevent.type=KeyPress; XSendEvent(Mydisplay, Mywindow, 1, KeyPressMask, (XEvent*)&thisrubishevent);
 Wait(3);
 XFlush(Mydisplay);
 Wait(1);
 XFlush(Mydisplay);
 XDestroyWindow(Mydisplay, Mywindow);
 XCloseDisplay(Mydisplay);
}
void Quit(){
 MyCleanup();
 printf("Quit\n");
 exit(0);
}

void Wait(int w){
 usleep(w*10000);
}
void Refresh(){
 if(!newbase_is_running)return;
 if( newbase_manual_refresh_mode ){
  XdbeSwapInfo swap_info = { Mywindow, LetsTryChangingThisAround };
  XdbeSwapBuffers(Mydisplay, &swap_info, 1);
 }
 XFlush(Mydisplay);
}

void CustomChar(unsigned char n, unsigned char b0,unsigned char b1,unsigned char b2,unsigned char b3,unsigned char b4,unsigned char b5,unsigned char b6,unsigned char b7){
 if( BBCFont == NULL ) return;
 BBCFont[n][0]=b0;
 BBCFont[n][1]=b1;
 BBCFont[n][2]=b2;
 BBCFont[n][3]=b3;
 BBCFont[n][4]=b4;
 BBCFont[n][5]=b5;
 BBCFont[n][6]=b6;
 BBCFont[n][7]=b7;
}
void DefaultFont(){
 if( BBCFont == NULL ) return;
 int i;
 for(i=0;i<256;i++){
  CustomChar(i,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55);
 }
 CustomChar(32,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0);
 CustomChar(33,0x18,0x18,0x18,0x18,0x18,0x0,0x18,0x0);
 CustomChar(34,0x6C,0x6C,0x6C,0x0,0x0,0x0,0x0,0x0);
 CustomChar(35,0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x0);
 CustomChar(36,0x18,0x3E,0x78,0x3C,0x1E,0x7C,0x18,0x0);
 CustomChar(37,0x62,0x66,0xC,0x18,0x30,0x66,0x46,0x0);
 CustomChar(38,0x70,0xD8,0xD8,0x70,0xDA,0xCC,0x76,0x0);
 CustomChar(39,0xC,0xC,0x18,0x0,0x0,0x0,0x0,0x0);
 CustomChar(40,0xC,0x18,0x30,0x30,0x30,0x18,0xC,0x0);
 CustomChar(41,0x30,0x18,0xC,0xC,0xC,0x18,0x30,0x0);
 CustomChar(42,0x44,0x6C,0x38,0xFE,0x38,0x6C,0x44,0x0);
 CustomChar(43,0x0,0x18,0x18,0x7E,0x18,0x18,0x0,0x0);
 CustomChar(44,0x0,0x0,0x0,0x0,0x0,0x18,0x18,0x30);
 CustomChar(45,0x0,0x0,0x0,0xFE,0x0,0x0,0x0,0x0);
 CustomChar(46,0x0,0x0,0x0,0x0,0x0,0x18,0x18,0x0);
 CustomChar(47,0x0,0x6,0xC,0x18,0x30,0x60,0x0,0x0);
 CustomChar(48,0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x0);
 CustomChar(49,0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x0);
 CustomChar(50,0x7C,0xC6,0xC,0x18,0x30,0x60,0xFE,0x0);
 CustomChar(51,0x7C,0xC6,0x6,0x1C,0x6,0xC6,0x7C,0x0);
 CustomChar(52,0x1C,0x3C,0x6C,0xCC,0xFE,0xC,0xC,0x0);
 CustomChar(53,0xFE,0xC0,0xFC,0x6,0x6,0xC6,0x7C,0x0);
 CustomChar(54,0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x0);
 CustomChar(55,0xFE,0x6,0xC,0x18,0x30,0x30,0x30,0x0);
 CustomChar(56,0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x0);
 CustomChar(57,0x7C,0xC6,0xC6,0x7E,0x6,0xC,0x78,0x0);
 CustomChar(58,0x0,0x0,0x18,0x18,0x0,0x18,0x18,0x0);
 CustomChar(59,0x0,0x0,0x18,0x18,0x0,0x18,0x18,0x30);
 CustomChar(60,0x6,0x1C,0x70,0xC0,0x70,0x1C,0x6,0x0);
 CustomChar(61,0x0,0x0,0xFE,0x0,0xFE,0x0,0x0,0x0);
 CustomChar(62,0xC0,0x70,0x1C,0x6,0x1C,0x70,0xC0,0x0);
 CustomChar(63,0x7C,0xC6,0xC6,0xC,0x18,0x0,0x18,0x0);
 CustomChar(64,0x7C,0xC6,0xDE,0xD6,0xDC,0xC0,0x7C,0x0);
 CustomChar(65,0x7C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x0);
 CustomChar(66,0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x0);
 CustomChar(67,0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x0);
 CustomChar(68,0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x0);
 CustomChar(69,0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x0);
 CustomChar(70,0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x0);
 CustomChar(71,0x7C,0xC6,0xC0,0xCE,0xC6,0xC6,0x7C,0x0);
 CustomChar(72,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x0);
 CustomChar(73,0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x0);
 CustomChar(74,0x3E,0xC,0xC,0xC,0xC,0xCC,0x78,0x0);
 CustomChar(75,0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x0);
 CustomChar(76,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x0);
 CustomChar(77,0xC6,0xEE,0xFE,0xD6,0xD6,0xC6,0xC6,0x0);
 CustomChar(78,0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x0);
 CustomChar(79,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x0);
 CustomChar(80,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x0);
 CustomChar(81,0x7C,0xC6,0xC6,0xC6,0xCA,0xCC,0x76,0x0);
 CustomChar(82,0xFC,0xC6,0xC6,0xFC,0xCC,0xC6,0xC6,0x0);
 CustomChar(83,0x7C,0xC6,0xC0,0x7C,0x6,0xC6,0x7C,0x0);
 CustomChar(84,0xFE,0x18,0x18,0x18,0x18,0x18,0x18,0x0);
 CustomChar(85,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x0);
 CustomChar(86,0xC6,0xC6,0x6C,0x6C,0x38,0x38,0x10,0x0);
 CustomChar(87,0xC6,0xC6,0xD6,0xD6,0xFE,0xEE,0xC6,0x0);
 CustomChar(88,0xC6,0x6C,0x38,0x10,0x38,0x6C,0xC6,0x0);
 CustomChar(89,0xC6,0xC6,0x6C,0x38,0x18,0x18,0x18,0x0);
 CustomChar(90,0xFE,0xC,0x18,0x30,0x60,0xC0,0xFE,0x0);
 CustomChar(91,0x7C,0x60,0x60,0x60,0x60,0x60,0x7C,0x0);
 CustomChar(92,0x0,0x60,0x30,0x18,0xC,0x6,0x0,0x0);
 CustomChar(93,0x3E,0x6,0x6,0x6,0x6,0x6,0x3E,0x0);
 CustomChar(94,0x10,0x38,0x6C,0xC6,0x82,0x0,0x0,0x0);
 CustomChar(95,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xFF);
 CustomChar(96,0x60,0x30,0x18,0x0,0x0,0x0,0x0,0x0);
 CustomChar(97,0x0,0x0,0x7C,0x6,0x7E,0xC6,0x7E,0x0);
 CustomChar(98,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x0);
 CustomChar(99,0x0,0x0,0x7C,0xC6,0xC0,0xC6,0x7C,0x0);
 CustomChar(100,0x6,0x6,0x7E,0xC6,0xC6,0xC6,0x7E,0x0);
 CustomChar(101,0x0,0x0,0x7C,0xC6,0xFE,0xC0,0x7C,0x0);
 CustomChar(102,0x3E,0x60,0x60,0xFC,0x60,0x60,0x60,0x0);
 CustomChar(103,0x0,0x0,0x7E,0xC6,0xC6,0x7E,0x6,0x7C);
 CustomChar(104,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x0);
 CustomChar(105,0x18,0x0,0x78,0x18,0x18,0x18,0x7E,0x0);
 CustomChar(106,0x18,0x0,0x38,0x18,0x18,0x18,0x18,0x70);
 CustomChar(107,0xC0,0xC0,0xC6,0xCC,0xF8,0xCC,0xC6,0x0);
 CustomChar(108,0x78,0x18,0x18,0x18,0x18,0x18,0x7E,0x0);
 CustomChar(109,0x0,0x0,0xEC,0xFE,0xD6,0xD6,0xC6,0x0);
 CustomChar(110,0x0,0x0,0xFC,0xC6,0xC6,0xC6,0xC6,0x0);
 CustomChar(111,0x0,0x0,0x7C,0xC6,0xC6,0xC6,0x7C,0x0);
 CustomChar(112,0x0,0x0,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0);
 CustomChar(113,0x0,0x0,0x7E,0xC6,0xC6,0x7E,0x6,0x7);
 CustomChar(114,0x0,0x0,0xDC,0xF6,0xC0,0xC0,0xC0,0x0);
 CustomChar(115,0x0,0x0,0x7E,0xC0,0x7C,0x6,0xFC,0x0);
 CustomChar(116,0x30,0x30,0xFC,0x30,0x30,0x30,0x1E,0x0);
 CustomChar(117,0x0,0x0,0xC6,0xC6,0xC6,0xC6,0x7E,0x0);
 CustomChar(118,0x0,0x0,0xC6,0xC6,0x6C,0x38,0x10,0x0);
 CustomChar(119,0x0,0x0,0xC6,0xD6,0xD6,0xFE,0xC6,0x0);
 CustomChar(120,0x0,0x0,0xC6,0x6C,0x38,0x6C,0xC6,0x0);
 CustomChar(121,0x0,0x0,0xC6,0xC6,0xC6,0x7E,0x6,0x7C);
 CustomChar(122,0x0,0x0,0xFE,0xC,0x38,0x60,0xFE,0x0);
 CustomChar(123,0xC,0x18,0x18,0x70,0x18,0x18,0xC,0x0);
 CustomChar(124,0x18,0x18,0x18,0x0,0x18,0x18,0x18,0x0);
 CustomChar(125,0x30,0x18,0x18,0xE,0x18,0x18,0x30,0x0);
 CustomChar(126,0x31,0x6B,0x46,0x0,0x0,0x0,0x0,0x0);
}

// ===========================================================

int MyColour(int r, int g, int b){
 r=r&255;
 g=g&255;
 b=b&255;
 switch(SDepth){
 case 24:
  return ((r&255)<<16) | ((g&255)<<8) | (b&255);
  break;
 case 16:
  return ((r>>3)<<11) | ((g>>3)<<6) | (b>>3);
  break;
 case 15:
  return ((r>>3)<<10) | ((g>>3)<<5) | (b>>3);
  break;
 default:
  return (r|g|b) > 127;
 }
}
int MyColour2(int col){
 int r,g,b;
 r=(col>>16)&0xff;
 g=(col>>8)&0xff;
 b=col&0xff;
 //printf("r%d g%d b%d\n",r,g,b);
 switch(SDepth){
 case 24:
  return ((r&255)<<16) | ((g&255)<<8) | (b&255);
  break;
 case 16:
  return ((r>>3)<<11) | ((g>>3)<<6) | (b>>3);
  break;
 case 15:
  return ((r>>3)<<10) | ((g>>3)<<5) | (b>>3);
  break;
 default:
  return (r|g|b) > 127;
 }
}

void Gcol(int r, int g, int b){
 if(!newbase_is_running)return;
 int c = MyColour(r,g,b);
 XSetForeground(Mydisplay,MyGC,c);
 NewBase_last_fg_col = c;
}

void GcolBG(int r, int g, int b){
 if(!newbase_is_running)return;
 int c = MyColour(r,g,b);
 //XSetBackground(Mydisplay,MyGC,c);
 //XSetWindowBackground(Mydisplay, Mywindow, c);
 NewBase_last_bg_col = c;
}

void GcolGrey(int g){
 if(!newbase_is_running)return;
 int c = MyColour(g,g,g);
 XSetForeground(Mydisplay,MyGC,c);
 NewBase_last_fg_col = c;
}

void GcolDirect(int rgb){
 if(!newbase_is_running)return;
 XSetForeground(Mydisplay,MyGC,rgb);
 NewBase_last_fg_col = rgb;
}
void GcolBGDirect(int rgb){
 if(!newbase_is_running)return;
 //XSetBackground(Mydisplay,MyGC,rgb);
 //XSetWindowBackground(Mydisplay, Mywindow, rgb);
 NewBase_last_bg_col = rgb;
}

void MySetWindowBackground(int r, int g, int b){
 if(!newbase_is_running)return;
 int c = MyColour(r,g,b);
 //XSetWindowBackground(Mydisplay,Mywindow,c);
 NewBase_last_bg_col = c;
}

void Rectangle(int x,int y,int w,int h){
 if(!newbase_is_running) return;
 if(w<0){
  x+=w; w=-w;
 }
 if(h<0){
  y+=h; h=-h;
 }
 XDrawRectangle(Mydisplay,Mydrawable,MyGC, x,y, w,h);
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}

void RectangleFill(int x,int y,int w,int h){
 if(!newbase_is_running) return;
 if(w<0){
  x+=w; w=-w;
 }
 if(h<0){
  y+=h; h=-h;
 }
 XFillRectangle(Mydisplay,Mydrawable,MyGC, x,y, w,h);
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}

void Line(int x,int y,int xb,int yb){
 if(!newbase_is_running) return;
 XDrawLine(Mydisplay, Mydrawable, MyGC, x,y, xb,yb);
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}

#define CircleArcMaxAngle 23040

void Circle(int x,int y, int r){
 if(!newbase_is_running) return;
 XDrawArc(Mydisplay, Mydrawable, MyGC, x-r/2, y-r/2, r, r, 0,CircleArcMaxAngle );
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}

void CircleFill(int x,int y, int r){
 if(!newbase_is_running) return;
 XFillArc(Mydisplay, Mydrawable, MyGC, x-r/2, y-r/2, r, r, 0,CircleArcMaxAngle );
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}

void Plot69(int x, int y){
 if(!newbase_is_running) return;
 XDrawPoint(Mydisplay, Mydrawable, MyGC, x,y);
}
/*
void Plot69_(int x, int y){
 if(!newbase_is_running) return;
 XDrawPoint(Mydisplay, Mydrawable, MyGC, x,y);
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}
*/

void Triangle(int x0,int y0, int x1,int y1, int x2,int y2){
 if(!newbase_is_running) return;
 XPoint tripoints[3];
 tripoints[0].x = x0;
 tripoints[0].y = y0;
 tripoints[1].x = x1;
 tripoints[1].y = y1;
 tripoints[2].x = x2;
 tripoints[2].y = y2;
 XFillPolygon(Mydisplay,Mydrawable,MyGC,tripoints, 3, Convex, CoordModeOrigin);
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
}

void Cls(){ // this only calls functions that check newbase_is_running 
 int c = NewBase_last_fg_col;
 GcolDirect( NewBase_last_bg_col );
 RectangleFill( 0,0,WinW,WinH );
 GcolDirect( c );
}

void Print(int x, int y,unsigned char *s){
 if(!newbase_is_running)return;
 int X,Y;
 while( *s != 0){
  for(X=0;X<8;X++){
   for(Y=0;Y<8;Y++){
    if( (BBCFont[*s][Y]) & (128>>X)  ) Plot69(x+X,y+Y);
   }
  }
#if 0
  XFlush(Mydisplay);
  Wait(6);
#endif
  x+=8; if( x > WinW ) return;
  s++;
 }
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
 return;
}
void Print2(int x, int y,unsigned char *s){
 if(!newbase_is_running)return;
 int X,Y;
 while( *s != 0){
  for(X=0;X<8;X++){
   for(Y=0;Y<8;Y++){
    if( (BBCFont[*s][Y]) & (128>>X)  ){ 
     Plot69(x+X,y+Y*2);
     Plot69(x+X,y+Y*2+1);
    }
   }
  }
#if 0
  XFlush(Mydisplay);
  Wait(6);
#endif
  x+=8; if( x > WinW ) return;
  s++;
 }
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
 return;
}

void Print3(int x, int y,unsigned char *s){
 if(!newbase_is_running)return;
 int X,Y;
 while( *s != 0){
  for(X=0;X<8;X++){
   for(Y=0;Y<8;Y++){
    if( (BBCFont[*s][Y]) & (128>>X)  ){ 
     Plot69(x+X*2,y+Y*2);
     Plot69(x+X*2,y+Y*2+1);
     Plot69(x+X*2+1,y+Y*2);
     Plot69(x+X*2+1,y+Y*2+1);
    }
   }
  }
#if 0
  XFlush(Mydisplay);
  Wait(6);
#endif
  x+=8*2; if( x > WinW ) return;
  s++;
 }
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
 return;
}

void Print4(int x, int y,unsigned char *s){
 if(!newbase_is_running)return;
 int X,Y;
 while( *s != 0){
  for(X=0;X<8;X++){
   for(Y=0;Y<8;Y++){
    if( (BBCFont[*s][Y]) & (128>>X)  ){ 
     Plot69(x+X*2,  y+Y);
     Plot69(x+X*2+1,y+Y);
    }
   }
  }
#if 0
  XFlush(Mydisplay);
  Wait(6);
#endif
  x+=8*2; if( x > WinW ) return;
  s++;
 }
 #ifdef NewBase_HaventRemovedThisYet
 if(xflush_for_every_draw)XFlush(Mydisplay);
 #endif
 return;
}

void drawtext_(int x, int y, int scale, unsigned char *s){ // this only calls functions that check newbase_is_running
 void (*PrintFunc)(int,int,unsigned char*);
 switch(scale&3){
 case 0b00: PrintFunc=Print;  break;
 case 0b01: PrintFunc=Print2; break;
 case 0b10: PrintFunc=Print4; break;
 case 0b11: PrintFunc=Print3; break;
 }
 PrintFunc(x,y,s);
}

// ============================================================
// ======== Drawing microsoft .bmp files to the screen ========
// ============================================================

typedef
union {
 struct {
  char identify[2]; //2
  unsigned int size; //4
  short reserved1; //2
  short reserved2; //2
  unsigned int offset; //4
  // the BITMAPINFOHEADER starts here
  unsigned int header_size; //4
  int width; //4
  int height;//4
  short colour_planes; // 2 
  short bits_per_pixel; //2
  int compression_method; //4
  unsigned int image_size; //4
  int horizontal_resolution; // 4
  int vertical_resolution;   // 4
  unsigned int number_of_colours_in_palette; //4
  int generally_ignored; //4
 } __attribute__((packed)) /* we must use this attribute to prevent GCC from padding the structure, so its layout in memory matches the bitmap file format */ 
   structure ;
 char rawdata[ 2 + 4 + 2 + 2 + 4 + 40 ];
} //end union
Bmp;

void NB_DrawBmp(int x, int y, int sx, int sy, int w, int h, Bmp *bmp){
 if(!newbase_is_running){
  return;
 }
 if( ! bmp ){
  printf("NB_DrawBmp: given null pointer\n");
  return;
 }
 if( (bmp->structure.identify[0] != 'B' || bmp->structure.identify[1] != 'M')
       ||
      bmp->structure.width < 0
       ||
      bmp->structure.height < 0
       ||
      bmp->structure.colour_planes != 1
       ||
      bmp->structure.compression_method != 0
       ||
      bmp->structure.bits_per_pixel != 24
       ||
      bmp->structure.image_size != (bmp->structure.width*bmp->structure.height*3 + bmp->structure.width%4*bmp->structure.height)
   )
 {
  printf("NB_DrawBmp: invalid bitmap\n");
  return;
 }
 if( w < 0 ) w = bmp->structure.width;
 if( h < 0 ) h = bmp->structure.height;
 char *image_data = malloc( bmp->structure.width * bmp->structure.height * 4 );
 int i,j,k,p;
 p=0;
 for(i=0; i<bmp->structure.height; i++){ // perform the necessary conversion of BMP image data, from BMP's bottom-to-top RGB+padding pixel array format, to RGBA top-to-bottom
  p = 4 * bmp->structure.width * (bmp->structure.height-i-1);
  for(j=0; j<bmp->structure.width; j++){
   for(k=0; k<3; k++){
    image_data[p++] = *((((char*)bmp->rawdata) + bmp->structure.offset) + bmp->structure.width*i*3 + 3*j + k + ( (bmp->structure.width%4*i) ) );
   }
   image_data[p++]=0;
  }
 }
 // create and initialise XImage structure with the converted image data, draw it onto the screen, and then free the memory allocated for the XImage
 XImage *image;
 image = XCreateImage(Mydisplay, DefaultVisual(Mydisplay,0), DefaultDepth(Mydisplay,0), ZPixmap, 0, image_data, bmp->structure.width, bmp->structure.height, 32, 0);
 image->byte_order = LSBFirst;
 image->bitmap_bit_order = LSBFirst;
 XInitImage(image);
 XPutImage(Mydisplay,Mywindow,MyGC,image, sx,sy,x,y, w, h);
 XDestroyImage(image); // XDestroyImage frees not only the XImage structure itself, but also the data pointer we put in the XImage struct, so image_data has already been free()'d
}

// ===================================================================
// ======== End of drawing microsoft .bmp files to the screen ========
// ===================================================================

// ===========================================================

unsigned char GetCharFromEvent(XEvent *ev){
 char string[25];
 int len;
 KeySym keysym;
 len = XLookupString(&ev->xkey, string, 25, &keysym, NULL);
 //printf("len %d strlen %d asc %d chr$ %c\n",len,strlen(string),(unsigned char)string[0],string[0]);
 if (len > 0 && len <= 1){
  //if (string[0] == '\r'){   string[0] = '\n';  }
  //fputs(string, stdout);
  return string[0];
 }
 return 0;
}

void PutOntoKeyboardBuffer(int c){ // keyboard buffer is statically allocated 
 if( ( (KeyboardBufferWritePos+1)%KeyBuffSize ) != KeyboardBufferReadPos ){
  KeyboardBuffer[ KeyboardBufferWritePos=(KeyboardBufferWritePos+1)%KeyBuffSize ]=c;
 }/*else{
  printf("keyboard buffer full, character lost: %d (%c)\n",c,c&0xff);
 }*/
}

int GET(){
 //while
 if( KeyboardBufferReadPos == KeyboardBufferWritePos ){
  return -1;
  //HandleEvents(); Wait(1);
 }
 return KeyboardBuffer[KeyboardBufferReadPos=(KeyboardBufferReadPos+1)%KeyBuffSize];
}

int KeyBufferFreeSpace(){
 return KeyboardBufferReadPos <= KeyboardBufferWritePos ? KeyBuffSize-(KeyboardBufferWritePos-KeyboardBufferReadPos) : (KeyboardBufferReadPos-KeyboardBufferWritePos);
}
int KeyBufferUsedSpace(){
 return KeyBuffSize-KeyBufferFreeSpace();
}

void ClearKeyboardBuffer(){
 KeyboardBufferReadPos=0; KeyboardBufferWritePos=0;
}

// ===========================================================



void SetClipRect(int x, int y, int w, int h){
 if(!newbase_is_running)return;
 XRectangle r;
 r.x=x; r.y=y;
 r.width=w; r.height=h;
 XSetClipRectangles(Mydisplay,MyGC,0,0,&r,1,Unsorted);
}
void ClearClipRect(){
 if(!newbase_is_running)return;
 XSetClipMask( Mydisplay, MyGC, None );
}

void SetPlottingMode(int function){
 if(!newbase_is_running)return;
 XSetFunction(Mydisplay,MyGC,function);
}
// ==========  Quick reference, taken from X11/X.h  =============
// GXclear		0x0		/* 0 */
// GXand		0x1		/* src AND dst */
// GXandReverse		0x2		/* src AND NOT dst */
// GXcopy		0x3		/* src */
// GXandInverted	0x4		/* NOT src AND dst */
// GXnoop		0x5		/* dst */
// GXxor		0x6		/* src XOR dst */
// GXor			0x7		/* src OR dst */
// GXnor		0x8		/* NOT src AND NOT dst */
// GXequiv		0x9		/* NOT src XOR dst */
// GXinvert		0xa		/* NOT dst */
// GXorReverse		0xb		/* src OR NOT dst */
// GXcopyInverted	0xc		/* NOT src */
// GXorInverted		0xd		/* NOT src OR dst */
// GXnand		0xe		/* NOT src OR NOT dst */
// GXset		0xf		/* 1 */


void RefreshOn(){
 if(!newbase_is_running) return;
 if( newbase_manual_refresh_mode ){
  newbase_manual_refresh_mode = 0;
  Wait(2);
  Mydrawable = Mywindow;
 }//endif
}//endproc

void RefreshOff(){
 if(!newbase_is_running) return;
 if( !newbase_manual_refresh_mode ){
  Wait(1);
  Mydrawable = Mybackbuffer;
  newbase_manual_refresh_mode = 1;
 }//endif
}//endproc






// ==============================================================================================================================
// ==============================================================================================================================




int NewBase_HandleEvents(int EnableBlocking){
 if(!newbase_is_running)return 0;
 int i;
 unsigned char ch;
 int DidSomethingHappen=0;
 int WindowWasResized=0;
 if( !CopyPasteOngoing && CopyPasteRequested ) NB_CopyPasteRequestHandler();
 //Refresh();
 while(XEventsQueued(Mydisplay,QueuedAlready) || EnableBlocking){
  EnableBlocking=0;
  DidSomethingHappen=1;
  //printf("XNextEvent\n");
  XNextEvent(Mydisplay, &Myevent);
  //printf("XNextEvent returned\n");

  //printf("event.type: %d\n",Myevent.type);

  switch(Myevent.type){
  case MapNotify:
   #if (GUI_DEBUG&0b1)
   printf("MapNotify\n");
   #endif
   break;
  case Expose:
   #if (GUI_DEBUG&0b1)
   printf("Expose\n");
   #endif
   exposed=1;
   break;
  case ConfigureNotify:
   #if (GUI_DEBUG&0b1)
   printf("ConfigureNotify\n");
   #endif
   //printf("COCK:\n x: %d\n y: %d\n w %d\n h%d\n\n",Myevent.xconfigure.x,Myevent.xconfigure.y,Myevent.xconfigure.width,Myevent.xconfigure.height);
   if( WinW != Myevent.xconfigure.width || WinH != Myevent.xconfigure.height ){
    WinW = Myevent.xconfigure.width;
    WinH = Myevent.xconfigure.height;
    WindowWasResized=1;
   }
   break;
  case KeyRelease:
  case KeyPress:
   #if (GUI_DEBUG&0b1)
   if( Myevent.type == KeyRelease ) printf("KeyRelease\n");
   if( Myevent.type == KeyPress   ) printf("KeyPress\n");
   #endif
   #if (GUI_DEBUG&0b1)
   printf("st: %d\n",Myevent.xkey.state); printf("kc: %d\n",Myevent.xkey.keycode);
   #endif
   ch=GetCharFromEvent(&Myevent);
   iskeypressed[Myevent.xkey.keycode&0xff] = (Myevent.type == KeyPress);
   if( Myevent.type == KeyPress /*&& ch*/ )PutOntoKeyboardBuffer( (0x80000000 * (Myevent.type == KeyRelease)) | ((Myevent.xkey.state&0xff)<<16) | ((Myevent.xkey.keycode&0xff)<<8) | ch );
   #if (GUI_DEBUG&0b1)
   printf("ch: %d\n",ch);
   #endif
   break;
  case ClientMessage: {
    #if (GUI_DEBUG&0b1)
    printf("ClientMessage\n");
    //system("Shia");
    #endif
    if( (Myevent.type == ClientMessage && (unsigned long) Myevent.xclient.data.l[0] == WmDeleteWindowAtom) ){
     if(wmcloseaction){
      wmclosed=1;
     }else{
      MyCleanup();
      exit(0);
     }
    }/*else{
     tb();
    }*/
   }
   break;
  case MotionNotify:
   #if (GUI_DEBUG&0b1)
   printf("MotionNotify\n");
   #endif
   mouse_x=Myevent.xmotion.x;
   mouse_y=Myevent.xmotion.y;
   break;
  case ButtonPress:
   {
   #if (GUI_DEBUG&0b1)
   printf("ButtonPress\n");
   #endif
   int buton = 1<<(Myevent.xbutton.button-1);
   switch(buton){
   case 8:
    mouse_z += 1;
    break;
   case 16:
    mouse_z -= 1;
    break;
   }
   mouse_b |= buton;
   //printf("buton %d\n",buton);
   }
   break;
  case ButtonRelease:
   #if (GUI_DEBUG&0b1)
   printf("ButtonRelease\n");
   #endif
   mouse_b = mouse_b & ~(1<<(Myevent.xbutton.button-1));
   break;
  case SelectionNotify:
   #if (GUI_DEBUG&0b1)
   printf("SelectionNotify\n");
   #endif
   {
    XSelectionEvent *sev = (XSelectionEvent*)&Myevent.xselection;
    NB_SelectionNotifyHandler( sev );
   }
   break;
  case SelectionRequest:
   #if (GUI_DEBUG&0b1)
   printf("SelectionRequest\n");
   #endif
   {
    NB_SelectionRequestHandler();
   }
   break;
  case PropertyNotify:
   #if (GUI_DEBUG&0b1)
   printf("PropertyNotify\n");
   #endif
   if( CopyPasteOngoing  ){
    NB_PropertyNotifyHandler();
   }
   break;
  default:
   #if (GUI_DEBUG&0b1)
   printf("NewBase event handler: unhandled: %d\n",Myevent.type);
   #endif
   break;
  }//endcase

 }//endwhile

 if( CopyPasteOngoing ){
  CopyPasteTimeoutCounter--;
  #if CopyPasteDebug
  printf("CopyPasteTimeoutCounter : %d\n",CopyPasteTimeoutCounter);
  #endif
  if( ! CopyPasteTimeoutCounter ){
   #if CopyPasteDebug
   printf("CopyPasteDebug: copy/paste INCR timeout occurred\n");
   #endif
   switch( CopyPasteOngoing ){
    case CPO__INCOMING_INCR: {
     if( PasteBuffer ){
      free( (void*)PasteBuffer );
      PasteBufferContentsSize = 0;
      PasteSucceeded = 0;
      CopyPasteRequested = 0;
      CopyPasteOngoing = 0;
     }
    } break;
    case CPO__OUTGOING_INCR: {
     CopyPasteOngoing = 0;
    } break;
   }//endcase
  }//endif timer ran out
 }//endif an incr copy/paste is happening

 //Refresh();
 return DidSomethingHappen;
}//endproc

// ==============================================================================================================================
// ==============================================================================================================================

void SetWindowSize(int w, int h){
 if(!newbase_is_running)return;
 XWindowChanges xwc;
 xwc.width = w;
 xwc.height = h;
 XConfigureWindow(Mydisplay,Mywindow, CWWidth | CWHeight, &xwc);
}

// ==============================================================================================================================
// ==============================================================================================================================

void* newbase_eventhandlerloopfunction(void *arg){
 while(newbase_is_running){
  NewBase_HandleEvents(0);
  if( ! CopyPasteRequested )
   Wait(1);
  else
   usleep(2);
  //usleep(16667);
  XFlush(Mydisplay);
 }
}
void start_newbase_thread(int w, int h){
 if(newbase_is_running) return;
 NewBase_MyInit(w,h,1);
 pthread_t newbasethread;
 pthread_create(&newbasethread, NULL, &newbase_eventhandlerloopfunction, NULL);
 while(!newbase_is_running){
  usleep(1000);
 }
 usleep(1000);
 SetWindowSize(w,h);
 Gcol(255,255,255);
 GcolBG(0,0,0);
 Cls();
 Wait(2);
}

#ifndef myxlib_notstandalonetest

int main(int argc, char **argv){

 start_newbase_thread(640,480);

 //Wait(1);
 //GcolBGDirect(0); // FUCK! setting a background colour makes it so that the window is automatically cleared whenever it's resized, results in bad flickering

 while(1){
  Gcol(Rnd(256),Rnd(256),Rnd(256));
  Circle(Rnd(WinW),Rnd(WinH),Rnd(WinH));
  Wait(1);
 }

 return 0;
}

#endif
