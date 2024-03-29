/*
	File:		DelegateOnlyTestApp.c
	
	Description: Some very basic code to test the Delegate Only Image Codec. Opens a .jpg image,
				 creats a '2vuy' offscreen and draws the image into the offscreen using Graphics Importers.
				 Once we have the source in the correct pixel format, we use a decompression sequence to draw
				 to the window. By changing the image description we force the ICM to call our Delegate Only Codec
				 which in turn just delegates everything to the apple '2vuy' codec.

	Author:		QuickTime DTS

	Copyright: 	� Copyright 2003 Apple Computer, Inc. All rights reserved.
	
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

#if !TARGET_API_MAC_CARBON
	#error "Carbon only"
#endif

#if __MACH__
    #include <Carbon/Carbon.h>
    #include <QuickTime/QuickTime.h>
#else
    #include <ConditionalMacros.h>
    #include <Fonts.h>
    #include <ImageCompression.h>
    #include <Movies.h>
    #include <NumberFormatting.h>
    #include <FixMath.h>
#endif

#include "GetFile.h"

// This app contains the delegate only component code, and 
// that component includes a register call which is prototyped here
extern void DelegateOnly_CodecRegister(void);

int main(void)
{
	WindowRef w = NULL;
	GraphicsImportComponent gi = NULL;
	Rect r = {100, 100, 130, 130};
	FSSpec theFSSpec;
	OSType fileType = FOUR_CHAR_CODE('JPEG');
	GWorldPtr myGWorld;
	OSErr err;
	
	// Initialize for Carbon and QuickTime		
	InitCursor();
	EnterMovies(); // yep, even for X

#if !STAND_ALONE
	// Register the built in Delegate Only component
	// for the Linked version of the TestApp
	DelegateOnly_CodecRegister();
#endif

	w = NewCWindow(NULL, &r, "\pTestApp", false, noGrowDocProc, (WindowPtr)-1, false, 0);
	if (NULL == w) goto bail;
	
	SetPortWindowPort(w);

	err = GetOneFileWithPreview(1, &fileType, &theFSSpec, NULL);
	if (err) goto bail;

	err = GetGraphicsImporterForFile(&theFSSpec, &gi);
	if (err) goto bail;

	err = GraphicsImportGetNaturalBounds(gi, &r);
	if (err) goto bail;

	SizeWindow(w, r.right - r.left, r.bottom - r.top, false);
	ShowWindow(w);
	
	// Create a '2vuy' offscreen for the test image, this is our "source"
	err = QTNewGWorld(&myGWorld, k2vuyPixelFormat, &r, NULL, NULL, 0);
	if (err) goto bail;
	
	GraphicsImportSetGWorld(gi, myGWorld, NULL); 

	// Draw it
	err = GraphicsImportDraw(gi);
	if (err) goto bail;
	
	{	// Now use the delegate component to draw to the screen
		Rect rSource;
		PixMapHandle hPixMap;
		Ptr pPixels;
		ImageSequence seqID = 0;
		ImageDescriptionHandle desc = NULL;
		
		GraphicsImportGetNaturalBounds(gi, &rSource);
		
		hPixMap = GetGWorldPixMap(myGWorld);
		LockPixels(hPixMap);
		
		err = MakeImageDescriptionForPixMap(hPixMap, &desc);
		if (err) goto bail;
		
		// change the image description so the ICM will call us
		(**desc).cType = FOUR_CHAR_CODE('DelO');
		
		pPixels	= GetPixBaseAddr(hPixMap);
		
		err = DecompressSequenceBegin(&seqID, desc, GetWindowPort(w), NULL, &rSource, NULL, srcCopy, NULL, 0, codecNormalQuality, NULL);
		if (err) goto bail;
		
		DecompressSequenceFrameS(seqID, pPixels, (**desc).dataSize, 0, NULL, NULL);
		
		CDSequenceEnd(seqID);
		DisposeHandle((Handle)desc);
	}
	
	while (!Button())
		;

bail:
	return 0;
}