/*
	File:		 DelegateOnly_Codec.c
	
	Description: Delegates everything to the '2vuy' codec. 
				 
	Author:		era

	Copyright: 	� Copyright 2003-2005 Apple Computer, Inc. All rights reserved.
	
	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
				
	Change History (most recent first):
*/

#if TARGET_OS_WIN32
    #include "ImportExampleStandAloneWin.h"
#endif

#if TARGET_CPU_68K
	#error "Stop it! You don't really want to build a 68k component do you?"
#endif

#if __MACH__
    #include <Carbon/Carbon.h>
    #include <QuickTime/QuickTime.h>
#else
    #include <ConditionalMacros.h>
    #include <Endian.h>
    #include <ImageCodec.h>
#endif

#define kDelegateOnly_CodecVersion (0x0001)

// Data structures
typedef struct	{
	ComponentInstance self;
	ComponentInstance delegateComponent;
	ComponentInstance target;
} DelegateOnly_GlobalsRecord, *DelegateOnly_Globals;

// Setup required for ComponentDispatchHelper.c
#define IMAGECODEC_BASENAME() 		DelegateOnly_ImageCodec
#define IMAGECODEC_GLOBALS() 		DelegateOnly_Globals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()

#define COMPONENT_UPP_SELECT_ROOT() ImageCodec
#define COMPONENT_DISPATCH_FILE		"DelegateOnly_CodecDispatch.h"

#define	GET_DELEGATE_COMPONENT()	(storage->delegateComponent)

#if __MACH__
	#include <CoreServices/Components.k.h>
	#include <QuickTime/ImageCodec.k.h>     // ComponentProperty selectors live here as well
	#include <QuickTime/ComponentDispatchHelper.c>
#else
	#include <Components.k.h>
	#include <ImageCodec.k.h>
	#include <ComponentDispatchHelper.c>
#endif

// Component Open Request - Required
pascal ComponentResult DelegateOnly_ImageCodecOpen(DelegateOnly_Globals glob, ComponentInstance self)
{
	ComponentDescription cd = { decompressorComponentType, k422YpCbCr8CodecType, FOUR_CHAR_CODE('app3'), 0, 0 };
	Component c = 0;
	
	ComponentResult rc;
	
	// Allocate memory for our globals, set them up and inform the component manager that we've done so
	glob = (DelegateOnly_Globals)NewPtrClear(sizeof(DelegateOnly_GlobalsRecord));
	if (rc = MemError()) goto bail;
	
	SetComponentInstanceStorage(self, (Handle)glob);
	
	glob->self = self;
	glob->target = self;
	
	if (c = FindNextComponent(c, &cd)) {
		
		rc = OpenAComponent(c, &glob->delegateComponent);
		if (rc) goto bail;

		ComponentSetTarget(glob->delegateComponent, self);
	}

bail:
	return rc;
}

// Component Close Request - Required
pascal ComponentResult DelegateOnly_ImageCodecClose(DelegateOnly_Globals glob, ComponentInstance self)
{
	// Make sure to close the component we opened and release memory
	if (glob) {
		if (glob->delegateComponent) {
			CloseComponent(glob->delegateComponent);
		}

		DisposePtr((Ptr)glob);
	}

	return noErr;
}

// Component Version Request - Required
pascal ComponentResult DelegateOnly_ImageCodecVersion(DelegateOnly_Globals glob)
{
#pragma unused(glob)

	return ((codecInterfaceVersion << 16) + kDelegateOnly_CodecVersion);
}

// Component Target Request
// 		Allows another component to "target" you i.e., you call another component whenever
// you would call yourself (as a result of your component being used by another component)
pascal ComponentResult DelegateOnly_ImageCodecTarget(DelegateOnly_Globals glob, ComponentInstance target)
{
	glob->target = target;
	
	return noErr;
}

#pragma mark-

// When building the *Application Version Only* make our component available for use by applications (or other clients).
// Once the Component Manager has registered a component, applications can find and open the component using standard
// Component Manager routines.
#if !STAND_ALONE && !TARGET_OS_WIN32
void DelegateOnly_CodecRegister(void);
void DelegateOnly_CodecRegister(void)
{
	ComponentDescription td;

	ComponentRoutineUPP componentEntryPoint = NewComponentRoutineUPP((ComponentRoutineProcPtr)DelegateOnly_ImageCodecComponentDispatch);

	td.componentType = decompressorComponentType;
	td.componentSubType = FOUR_CHAR_CODE('DelO');		// remember to change the subType and manufacturer
	td.componentManufacturer = FOUR_CHAR_CODE('DTS ');  // for your application - Apple reserves all lowercase types
	td.componentFlags = codecInfoDoes32;
	td.componentFlagsMask = 0;

	RegisterComponent(&td, componentEntryPoint, 0, NULL, NULL, NULL);
}
#endif // !STAND_ALONE && TARGET_OS_WIN32