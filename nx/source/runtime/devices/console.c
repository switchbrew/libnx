#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>
#include "runtime/devices/console.h"
#include "kernel/svc.h"
#include "display/gfx.h"

#include "default_font_bin.h"

//set up the palette for color printing
static u32 colorTable[] = {
	RGBA8_MAXALPHA(  0,  0,  0),	// black
	RGBA8_MAXALPHA(128,  0,  0),	// red
	RGBA8_MAXALPHA(  0,128,  0),	// green
	RGBA8_MAXALPHA(128,128,  0),	// yellow
	RGBA8_MAXALPHA(  0,  0,128),	// blue
	RGBA8_MAXALPHA(128,  0,128),	// magenta
	RGBA8_MAXALPHA(  0,128,128),	// cyan
	RGBA8_MAXALPHA(192,192,192),	// white

	RGBA8_MAXALPHA(128,128,128),	// bright black
	RGBA8_MAXALPHA(255,  0,  0),	// bright red
	RGBA8_MAXALPHA(  0,255,  0),	// bright green
	RGBA8_MAXALPHA(255,255,  0),	// bright yellow
	RGBA8_MAXALPHA(  0,  0,255),	// bright blue
	RGBA8_MAXALPHA(255,  0,255),	// bright magenta
	RGBA8_MAXALPHA(  0,255,255),	// bright cyan
	RGBA8_MAXALPHA(255,255,255),	// bright white

	RGBA8_MAXALPHA(  0,  0,  0),	// faint black
	RGBA8_MAXALPHA( 64,  0,  0),	// faint red
	RGBA8_MAXALPHA(  0, 64,  0),	// faint green
	RGBA8_MAXALPHA( 64, 64,  0),	// faint yellow
	RGBA8_MAXALPHA(  0,  0, 64),	// faint blue
	RGBA8_MAXALPHA( 64,  0, 64),	// faint magenta
	RGBA8_MAXALPHA(  0, 64, 64),	// faint cyan
	RGBA8_MAXALPHA( 96, 96, 96),	// faint white
};

//The below width/height is for 720p.
PrintConsole defaultConsole =
{
	//Font:
	{
		(u16*)default_font_bin, //font gfx
		0, //first ascii character in the set
		256 //number of characters in the font set
	},
	(u32*)NULL,
	(u32*)NULL,
	0,0,	//cursorX cursorY
	0,0,	//prevcursorX prevcursorY
	80,		//console width
	45,		//console height
	0,		//window x
	0,		//window y
	80,		//window width
	45,		//window height
	3,		//tab size
	7,		// foreground color
	0,		// background color
	0,		// flags
	0,		//print callback
	false	//console initialized
};

PrintConsole currentCopy;

PrintConsole* currentConsole = &currentCopy;

PrintConsole* consoleGetDefault(void){return &defaultConsole;}

void consolePrintChar(int c);
void consoleDrawChar(int c);

//---------------------------------------------------------------------------------
static void consoleCls(char mode) {
//---------------------------------------------------------------------------------

	int i = 0;
	int colTemp,rowTemp;

	switch (mode)
	{
	case '[':
	case '0':
		{
			colTemp = currentConsole->cursorX ;
			rowTemp = currentConsole->cursorY ;

			while(i++ < ((currentConsole->windowHeight * currentConsole->windowWidth) - (rowTemp * currentConsole->consoleWidth + colTemp)))
				consolePrintChar(' ');

			currentConsole->cursorX  = colTemp;
			currentConsole->cursorY  = rowTemp;
			break;
		}
	case '1':
		{
			colTemp = currentConsole->cursorX ;
			rowTemp = currentConsole->cursorY ;

			currentConsole->cursorY  = 0;
			currentConsole->cursorX  = 0;

			while (i++ < (rowTemp * currentConsole->windowWidth + colTemp))
				consolePrintChar(' ');

			currentConsole->cursorX  = colTemp;
			currentConsole->cursorY  = rowTemp;
			break;
		}
	case '2':
		{
			currentConsole->cursorY  = 0;
			currentConsole->cursorX  = 0;

			while(i++ < currentConsole->windowHeight * currentConsole->windowWidth)
				consolePrintChar(' ');

			currentConsole->cursorY  = 0;
			currentConsole->cursorX  = 0;
			break;
		}
	}
	gfxFlushBuffers();
	gfxSwapBuffers();
	gfxWaitForVsync();
}
//---------------------------------------------------------------------------------
static void consoleClearLine(char mode) {
//---------------------------------------------------------------------------------

	int i = 0;
	int colTemp;

	switch (mode)
	{
	case '[':
	case '0':
		{
			colTemp = currentConsole->cursorX ;

			while(i++ < (currentConsole->windowWidth - colTemp)) {
				consolePrintChar(' ');
			}

			currentConsole->cursorX  = colTemp;

			break;
		}
	case '1':
		{
			colTemp = currentConsole->cursorX ;

			currentConsole->cursorX  = 0;

			while(i++ < ((currentConsole->windowWidth - colTemp)-2)) {
				consolePrintChar(' ');
			}

			currentConsole->cursorX  = colTemp;

			break;
		}
	case '2':
		{
			colTemp = currentConsole->cursorX ;

			currentConsole->cursorX  = 0;

			while(i++ < currentConsole->windowWidth) {
				consolePrintChar(' ');
			}

			currentConsole->cursorX  = colTemp;

			break;
		}
	}
	gfxFlushBuffers();
	gfxSwapBuffers();
	gfxWaitForVsync();
}


//---------------------------------------------------------------------------------
static inline void consolePosition(int x, int y) {
//---------------------------------------------------------------------------------
	// invalid position
	if(x < 0 || y < 0)
		return;

	// 1-based, but we'll take a 0
	if(x < 1)
		x = 1;
	if(y < 1)
		y = 1;

	// clip to console edge
	if(x > currentConsole->windowWidth)
		x = currentConsole->windowWidth;
	if(y > currentConsole->windowHeight)
		y = currentConsole->windowHeight;

	// 1-based adjustment
	currentConsole->cursorX = x - 1;
	currentConsole->cursorY = y - 1;
}

//---------------------------------------------------------------------------------
static ssize_t con_write(struct _reent *r,void *fd,const char *ptr, size_t len) {
//---------------------------------------------------------------------------------

	char chr;

	int i, count = 0;
	char *tmp = (char*)ptr;

	if(!tmp) return -1;

	i = 0;

	while(i<len) {

		chr = *(tmp++);
		i++; count++;

		if ( chr == 0x1b && *tmp == '[' ) {
			bool escaping = true;
			char *escapeseq	= tmp++;
			int escapelen = 1;
			i++; count++;

			do {
				chr = *(tmp++);
				i++; count++; escapelen++;
				int parameter, assigned, consumed;

				// make sure parameters are positive values and delimited by semicolon
				if((chr >= '0' && chr <= '9') || chr == ';')
					continue;

				switch (chr) {
					//---------------------------------------
					// Cursor directional movement
					//---------------------------------------
					case 'A':
						consumed = 0;
						assigned = sscanf(escapeseq,"[%dA%n", &parameter, &consumed);
						if (assigned==0) parameter = 1;
						if (consumed)
							currentConsole->cursorY  =  (currentConsole->cursorY  - parameter) < 0 ? 0 : currentConsole->cursorY  - parameter;
						escaping = false;
						break;
					case 'B':
						consumed = 0;
						assigned = sscanf(escapeseq,"[%dB%n", &parameter, &consumed);
						if (assigned==0) parameter = 1;
						if (consumed)
							currentConsole->cursorY  =  (currentConsole->cursorY  + parameter) > currentConsole->windowHeight - 1 ? currentConsole->windowHeight - 1 : currentConsole->cursorY  + parameter;
						escaping = false;
						break;
					case 'C':
						consumed = 0;
						assigned = sscanf(escapeseq,"[%dC%n", &parameter, &consumed);
						if (assigned==0) parameter = 1;
						if (consumed)
							currentConsole->cursorX  =  (currentConsole->cursorX  + parameter) > currentConsole->windowWidth - 1 ? currentConsole->windowWidth - 1 : currentConsole->cursorX  + parameter;
						escaping = false;
						break;
					case 'D':
						consumed = 0;
						assigned = sscanf(escapeseq,"[%dD%n", &parameter, &consumed);
						if (assigned==0) parameter = 1;
						if (consumed)
							currentConsole->cursorX  =  (currentConsole->cursorX  - parameter) < 0 ? 0 : currentConsole->cursorX  - parameter;
						escaping = false;
						break;
					//---------------------------------------
					// Cursor position movement
					//---------------------------------------
					case 'H':
					case 'f':
					{
						int  x, y;
						char c;
						if(sscanf(escapeseq,"[%d;%d%c", &y, &x, &c) == 3 && (c == 'f' || c == 'H')) {
							consolePosition(x, y);
							escaping = false;
							break;
						}

						x = y = 1;
						if(sscanf(escapeseq,"[%d;%c", &y, &c) == 2 && (c == 'f' || c == 'H')) {
							consolePosition(x, y);
							escaping = false;
							break;
						}

						x = y = 1;
						if(sscanf(escapeseq,"[;%d%c", &x, &c) == 2 && (c == 'f' || c == 'H')) {
							consolePosition(x, y);
							escaping = false;
							break;
						}

						x = y = 1;
						if(sscanf(escapeseq,"[;%c", &c) == 1 && (c == 'f' || c == 'H')) {
							consolePosition(x, y);
							escaping = false;
							break;
						}

						// invalid format
						escaping = false;
						break;
					}
					//---------------------------------------
					// Screen clear
					//---------------------------------------
					case 'J':
						if(escapelen <= 3)
							consoleCls(escapeseq[escapelen-2]);
						escaping = false;
						break;
					//---------------------------------------
					// Line clear
					//---------------------------------------
					case 'K':
						if(escapelen <= 3)
							consoleClearLine(escapeseq[escapelen-2]);
						escaping = false;
						break;
					//---------------------------------------
					// Save cursor position
					//---------------------------------------
					case 's':
						if(escapelen == 2) {
							currentConsole->prevCursorX  = currentConsole->cursorX ;
							currentConsole->prevCursorY  = currentConsole->cursorY ;
						}
						escaping = false;
						break;
					//---------------------------------------
					// Load cursor position
					//---------------------------------------
					case 'u':
						if(escapelen == 2) {
							currentConsole->cursorX  = currentConsole->prevCursorX ;
							currentConsole->cursorY  = currentConsole->prevCursorY ;
						}
						escaping = false;
						break;
					//---------------------------------------
					// Color scan codes
					//---------------------------------------
					case 'm':
						escapeseq++;
						escapelen--;

						do {
							parameter = 0;
							if (escapelen == 1) {
								consumed = 1;
							} else if (memchr(escapeseq,';',escapelen)) {
								sscanf(escapeseq,"%d;%n", &parameter, &consumed);
							} else {
								sscanf(escapeseq,"%dm%n", &parameter, &consumed);
							}

							escapeseq += consumed;
							escapelen -= consumed;

							switch(parameter) {
							case 0: // reset
								currentConsole->flags = 0;
								currentConsole->bg    = 0;
								currentConsole->fg    = 7;
								break;

							case 1: // bold
								currentConsole->flags &= ~CONSOLE_COLOR_FAINT;
								currentConsole->flags |= CONSOLE_COLOR_BOLD;
								break;

							case 2: // faint
								currentConsole->flags &= ~CONSOLE_COLOR_BOLD;
								currentConsole->flags |= CONSOLE_COLOR_FAINT;
								break;

							case 3: // italic
								currentConsole->flags |= CONSOLE_ITALIC;
								break;

							case 4: // underline
								currentConsole->flags |= CONSOLE_UNDERLINE;
								break;

							case 5: // blink slow
								currentConsole->flags &= ~CONSOLE_BLINK_FAST;
								currentConsole->flags |= CONSOLE_BLINK_SLOW;
								break;

							case 6: // blink fast
								currentConsole->flags &= ~CONSOLE_BLINK_SLOW;
								currentConsole->flags |= CONSOLE_BLINK_FAST;
								break;

							case 7: // reverse video
								currentConsole->flags |= CONSOLE_COLOR_REVERSE;
								break;

							case 8: // conceal
								currentConsole->flags |= CONSOLE_CONCEAL;
								break;

							case 9: // crossed-out
								currentConsole->flags |= CONSOLE_CROSSED_OUT;
								break;

							case 21: // bold off
								currentConsole->flags &= ~CONSOLE_COLOR_BOLD;
								break;

							case 22: // normal color
								currentConsole->flags &= ~CONSOLE_COLOR_BOLD;
								currentConsole->flags &= ~CONSOLE_COLOR_FAINT;
								break;

							case 23: // italic off
								currentConsole->flags &= ~CONSOLE_ITALIC;
								break;

							case 24: // underline off
								currentConsole->flags &= ~CONSOLE_UNDERLINE;
								break;

							case 25: // blink off
								currentConsole->flags &= ~CONSOLE_BLINK_SLOW;
								currentConsole->flags &= ~CONSOLE_BLINK_FAST;
								break;

							case 27: // reverse off
								currentConsole->flags &= ~CONSOLE_COLOR_REVERSE;
								break;

							case 29: // crossed-out off
								currentConsole->flags &= ~CONSOLE_CROSSED_OUT;
								break;

							case 30 ... 37: // writing color
								currentConsole->fg = parameter - 30;
								break;

							case 39: // reset foreground color
								currentConsole->fg = 7;
								break;

							case 40 ... 47: // screen color
								currentConsole->bg = parameter - 40;
								break;

							case 49: // reset background color
								currentConsole->fg = 0;
								break;
							}
						} while (escapelen > 0);

						escaping = false;
						break;

					default:
						// some sort of unsupported escape; just gloss over it
						escaping = false;
						break;
				}
			} while (escaping);
			continue;
		}

		consolePrintChar(chr);
	}

	return count;
}

static const devoptab_t dotab_stdout = {
	"con",
	0,
	NULL,
	NULL,
	con_write,
	NULL,
	NULL,
	NULL
};

//---------------------------------------------------------------------------------
static ssize_t debug_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
//---------------------------------------------------------------------------------
	svcOutputDebugString(ptr,len);
	return len;
}

static const devoptab_t dotab_svc = {
	"svc",
	0,
	NULL,
	NULL,
	debug_write,
	NULL,
	NULL,
	NULL
};


static const devoptab_t dotab_null = {
	"null",
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

//---------------------------------------------------------------------------------
PrintConsole* consoleInit(PrintConsole* console) {
//---------------------------------------------------------------------------------

	static bool firstConsoleInit = true;

	if(firstConsoleInit) {
		devoptab_list[STD_OUT] = &dotab_stdout;
		devoptab_list[STD_ERR] = &dotab_stdout;

		setvbuf(stdout, NULL , _IONBF, 0);
		setvbuf(stderr, NULL , _IONBF, 0);

		firstConsoleInit = false;
	}

	if(console) {
		currentConsole = console;
	} else {
		console = currentConsole;
	}

	*currentConsole = defaultConsole;

	console->consoleInitialised = 1;

	gfxSetMode(GfxMode_TiledDouble);

	console->frameBuffer  = (u32*)gfxGetFramebuffer(NULL, NULL);
	gfxSwapBuffers();
	console->frameBuffer2 = (u32*)gfxGetFramebuffer(NULL, NULL);

	gfxFlushBuffers();
	gfxSwapBuffers();
	gfxWaitForVsync();

	consoleCls('2');

	return currentConsole;

}

//---------------------------------------------------------------------------------
void consoleDebugInit(debugDevice device){
//---------------------------------------------------------------------------------

	int buffertype = _IONBF;

	switch(device) {

	case debugDevice_SVC:
		devoptab_list[STD_ERR] = &dotab_svc;
		buffertype = _IOLBF;
		break;
	case debugDevice_CONSOLE:
		devoptab_list[STD_ERR] = &dotab_stdout;
		break;
	case debugDevice_NULL:
		devoptab_list[STD_ERR] = &dotab_null;
		break;
	}
	setvbuf(stderr, NULL , buffertype, 0);

}

//---------------------------------------------------------------------------------
PrintConsole *consoleSelect(PrintConsole* console){
//---------------------------------------------------------------------------------
	PrintConsole *tmp = currentConsole;
	currentConsole = console;
	return tmp;
}

//---------------------------------------------------------------------------------
void consoleSetFont(PrintConsole* console, ConsoleFont* font){
//---------------------------------------------------------------------------------

	if(!console) console = currentConsole;

	console->font = *font;

}

//---------------------------------------------------------------------------------
static void newRow(void) {
//---------------------------------------------------------------------------------


	currentConsole->cursorY ++;


	if(currentConsole->cursorY  >= currentConsole->windowHeight)  {
		currentConsole->cursorY --;

		int i,j;
		u32 x, y;

		x = currentConsole->windowX * 16;
		y = currentConsole->windowY * 16;

		for (i=0; i<currentConsole->windowWidth*16; i++) {
			u32 *from;
			u32 *to;
			for (j=0;j<(currentConsole->windowHeight-1)*16;j++) {
				to = &currentConsole->frameBuffer[gfxGetFramebufferDisplayOffset(x + i, y + j)];
				from = &currentConsole->frameBuffer[gfxGetFramebufferDisplayOffset(x + i, y + 16 + j)];
				*to = *from;
				to = &currentConsole->frameBuffer2[gfxGetFramebufferDisplayOffset(x + i, y + j)];
				from = &currentConsole->frameBuffer2[gfxGetFramebufferDisplayOffset(x + i, y + 16 + j)];
				*to = *from;
			}
		}

		consoleClearLine('2');
	}
}
//---------------------------------------------------------------------------------
void consoleDrawChar(int c) {
//---------------------------------------------------------------------------------
	c -= currentConsole->font.asciiOffset;
	if ( c < 0 || c > currentConsole->font.numChars ) return;

	u16 *fontdata = currentConsole->font.gfx + (16 * c);

	int writingColor = currentConsole->fg;
	int screenColor = currentConsole->bg;

	if (currentConsole->flags & CONSOLE_COLOR_BOLD) {
		writingColor += 8;
	} else if (currentConsole->flags & CONSOLE_COLOR_FAINT) {
		writingColor += 16;
	}

	if (currentConsole->flags & CONSOLE_COLOR_REVERSE) {
		int tmp = writingColor;
		writingColor = screenColor;
		screenColor = tmp;
	}

	u32 bg = colorTable[screenColor];
	u32 fg = colorTable[writingColor];

	u128 *tmp = (u128*)fontdata;

	u128 bvaltop = tmp[0];
	u128 bvalbtm = tmp[1];

	if (currentConsole->flags & CONSOLE_UNDERLINE)  bvalbtm |= (u128)0xffffULL << 7*16;

	if (currentConsole->flags & CONSOLE_CROSSED_OUT) bvaltop |= (u128)0xffffULL << 7*16;

	u16 mask = 0x8000;

	int i, j;

	int x = (currentConsole->cursorX + currentConsole->windowX) * 16;
	int y = ((currentConsole->cursorY + currentConsole->windowY) *16 );

	u32 *screen;

	for (i=0;i<16;i++) {
		for (j=0;j<8;j++) {
			uint32_t screenOffset = gfxGetFramebufferDisplayOffset(x + i, y + j);
			screen = &currentConsole->frameBuffer[screenOffset];
			if (bvaltop >> (16*j) & mask) { *screen = fg; }else{ *screen = bg; }
			screen = &currentConsole->frameBuffer2[screenOffset];
			if (bvaltop >> (16*j) & mask) { *screen = fg; }else{ *screen = bg; }

			screenOffset = gfxGetFramebufferDisplayOffset(x + i, y + j + 8);
			screen = &currentConsole->frameBuffer[screenOffset];
			if (bvalbtm >> (16*j) & mask) { *screen = fg; }else{ *screen = bg; }
			screen = &currentConsole->frameBuffer2[screenOffset];
			if (bvalbtm >> (16*j) & mask) { *screen = fg; }else{ *screen = bg; }
		}
		mask >>= 1;
	}

}

//---------------------------------------------------------------------------------
void consolePrintChar(int c) {
//---------------------------------------------------------------------------------
	if (c==0) return;

	if(currentConsole->PrintChar)
		if(currentConsole->PrintChar(currentConsole, c))
			return;

	if(currentConsole->cursorX  >= currentConsole->windowWidth) {
		currentConsole->cursorX  = 0;

		newRow();
	}

	switch(c) {
		/*
		The only special characters we will handle are tab (\t), carriage return (\r), line feed (\n)
		and backspace (\b).
		Carriage return & line feed will function the same: go to next line and put cursor at the beginning.
		For everything else, use VT sequences.

		Reason: VT sequences are more specific to the task of cursor placement.
		The special escape sequences \b \f & \v are archaic and non-portable.
		*/
		case 8:
			currentConsole->cursorX--;

			if(currentConsole->cursorX < 0) {
				if(currentConsole->cursorY > 0) {
					currentConsole->cursorX = currentConsole->windowX - 1;
					currentConsole->cursorY--;
				} else {
					currentConsole->cursorX = 0;
				}
			}

			consoleDrawChar(' ');
			break;

		case 9:
			currentConsole->cursorX  += currentConsole->tabSize - ((currentConsole->cursorX)%(currentConsole->tabSize));
			break;
		case 10:
			newRow();
		case 13:
			currentConsole->cursorX  = 0;
			gfxFlushBuffers();
			gfxSwapBuffers();
			gfxWaitForVsync();
			break;
		default:
			consoleDrawChar(c);
			++currentConsole->cursorX ;
			break;
	}
}

//---------------------------------------------------------------------------------
void consoleClear(void) {
//---------------------------------------------------------------------------------
	iprintf("\x1b[2J");
}

//---------------------------------------------------------------------------------
void consoleSetWindow(PrintConsole* console, int x, int y, int width, int height){
//---------------------------------------------------------------------------------

	if(!console) console = currentConsole;

	console->windowWidth = width;
	console->windowHeight = height;
	console->windowX = x;
	console->windowY = y;

	console->cursorX = 0;
	console->cursorY = 0;

}



