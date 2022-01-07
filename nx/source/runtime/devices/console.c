#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>
#include "runtime/devices/console.h"
#include "display/framebuffer.h"

#include "default_font_bin.h"

static const u8 colorCube[] = {
	0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff,
};

static const u8 grayScale[] = {
	0x08, 0x12, 0x1c, 0x26, 0x30, 0x3a, 0x44, 0x4e,
	0x58, 0x62, 0x6c, 0x76, 0x80, 0x8a, 0x94, 0x9e,
	0xa8, 0xb2, 0xbc, 0xc6, 0xd0, 0xda, 0xe4, 0xee,
};

//The below width/height is for 720p.
static PrintConsole defaultConsole =
{
	//Font:
	{
		default_font_bin, //font gfx
		0, //first ascii character in the set
		256, //number of characters in the font set
		16, //tile width
		16, //tile height
	},
	NULL,   //renderer
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
	false	//console initialized
};

static bool parseColor (char **esc, int *escLen, u16 *color, bool *custom)
{
	unsigned int p;
	unsigned int n;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	int consumed;

	if (sscanf (*esc, "%d;%n", &p, &consumed) != 1)
		return false;

	*esc    += consumed;
	*escLen -= consumed;

	if (p == 5) {
		if (sscanf (*esc, "%u%n", &n, &consumed) != 1)
			return false;

		*esc    += consumed;
		*escLen -= consumed;

		if (n <= 15) {
			*color  = n;
			*custom = false;
		} else if (n <= 231) {
			n -= 16;
			r = n / 36;
			g = (n - r * 36) / 6;
			b = n - r * 36 - g * 6;

			*color  = RGB565_FROM_RGB8 (colorCube[r], colorCube[g], colorCube[b]);
			*custom = true;
		} else if (n <= 255) {
			n -= 232;

			*color  = RGB565_FROM_RGB8 (grayScale[n], grayScale[n], grayScale[n]);
			*custom = true;
		} else {
			return false;
		}

		return true;
	} else if (p == 2) {
		if (sscanf (*esc, "%u;%u;%u%n", &r, &g, &b, &consumed) != 3)
			return false;

		*esc    += consumed;
		*escLen -= consumed;

		*color  = RGB565_FROM_RGB8 (r, g, b);
		*custom = true;

		return true;
	}

	return false;
}

static PrintConsole currentCopy;

static PrintConsole* currentConsole = &currentCopy;

PrintConsole* consoleGetDefault(void){return &defaultConsole;}

static void consoleNewRow(void);
static void consolePrintChar(int c);
static void consoleDrawChar(int c);

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

		if ( chr == 0x1b && len > 1 && *tmp == '[' ) {
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
							bool custom;
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
								currentConsole->flags &= ~CONSOLE_FG_CUSTOM;
								currentConsole->fg     = parameter - 30;
								break;

							case 38: // custom foreground color
								if (parseColor (&escapeseq, &escapelen, &currentConsole->fg, &custom)) {
									if (custom)
										currentConsole->flags |= CONSOLE_FG_CUSTOM;
									else
										currentConsole->flags &= ~CONSOLE_FG_CUSTOM;

									if (!custom && currentConsole->fg < 16) {
										currentConsole->flags &= ~CONSOLE_COLOR_FAINT;
										if (currentConsole->fg < 8)
											currentConsole->flags &= ~CONSOLE_COLOR_BOLD;
										else
											currentConsole->flags |= CONSOLE_COLOR_BOLD;
									}

									// consume next ; or m
									++escapeseq;
									--escapelen;
								} else {
									// stop processing
									escapelen = 0;
								}
								break;

							case 39: // reset foreground color
								currentConsole->flags &= ~CONSOLE_FG_CUSTOM;
								currentConsole->fg     = 7;
								break;

							case 40 ... 47: // screen color
								currentConsole->flags &= ~CONSOLE_BG_CUSTOM;
								currentConsole->bg     = parameter - 40;
								break;

							case 48: // custom background color
								if (parseColor (&escapeseq, &escapelen, &currentConsole->bg, &custom)) {
									if (custom)
										currentConsole->flags |= CONSOLE_BG_CUSTOM;
									else
										currentConsole->flags &= ~CONSOLE_BG_CUSTOM;

									// consume next ; or m
									++escapeseq;
									--escapelen;
								} else {
									// stop processing
									escapelen = 0;
								}
								break;

							case 49: // reset background color
								currentConsole->flags &= ~CONSOLE_BG_CUSTOM;
								currentConsole->bg     = 0;
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
	.name    = "con",
	.write_r = con_write,
};

const devoptab_t* __nx_get_console_dotab(void) {
	return &dotab_stdout;
}

ConsoleRenderer* getDefaultConsoleRenderer(void);

//---------------------------------------------------------------------------------
PrintConsole* consoleInit(PrintConsole* console) {
//---------------------------------------------------------------------------------

	static bool didFirstConsoleInit = false;

	if(!didFirstConsoleInit) {
		devoptab_list[STD_OUT] = &dotab_stdout;
		setvbuf(stdout, NULL, _IONBF, 0);
		didFirstConsoleInit = true;
	}

	if(console) {
		currentConsole = console;
	} else {
		console = currentConsole;
	}

	*currentConsole = defaultConsole;
	if (!console->renderer) {
		console->renderer = getDefaultConsoleRenderer();
	}

	if (!console->consoleInitialised && console->renderer->init(console)) {
		console->consoleInitialised = true;
		consoleCls('2');
		return console;
	}

	return currentConsole;
}

//---------------------------------------------------------------------------------
void consoleExit(PrintConsole* console) {
//---------------------------------------------------------------------------------

	if (!console) console = currentConsole;

	if (console->consoleInitialised) {
		console->renderer->deinit(console);
		console->consoleInitialised = false;
	}
}

//---------------------------------------------------------------------------------
void consoleUpdate(PrintConsole* console) {
//---------------------------------------------------------------------------------

	if (!console) console = currentConsole;

	if (console->consoleInitialised) {
		console->renderer->flushAndSwap(console);
	}
}

//---------------------------------------------------------------------------------
PrintConsole *consoleSelect(PrintConsole* console) {
//---------------------------------------------------------------------------------
	PrintConsole *tmp = currentConsole;
	currentConsole = console;
	return tmp;
}

//---------------------------------------------------------------------------------
void consoleSetFont(PrintConsole* console, ConsoleFont* font) {
//---------------------------------------------------------------------------------

	if(!console) console = currentConsole;

	console->font = *font;

}

//---------------------------------------------------------------------------------
void consoleNewRow(void) {
//---------------------------------------------------------------------------------
	currentConsole->cursorY ++;

	if(currentConsole->cursorY  >= currentConsole->windowHeight)  {
		currentConsole->cursorY --;
		currentConsole->renderer->scrollWindow(currentConsole);
		consoleClearLine('2');
	}
}

//---------------------------------------------------------------------------------
void consoleDrawChar(int c) {
//---------------------------------------------------------------------------------
	c -= currentConsole->font.asciiOffset;
	if ( c < 0 || c > currentConsole->font.numChars ) return;

	currentConsole->renderer->drawChar(
		currentConsole,
		currentConsole->cursorX + currentConsole->windowX,
		currentConsole->cursorY + currentConsole->windowY,
		c);
}

//---------------------------------------------------------------------------------
void consolePrintChar(int c) {
//---------------------------------------------------------------------------------
	if (c==0) return;

	if(currentConsole->cursorX  >= currentConsole->windowWidth) {
		currentConsole->cursorX  = 0;

		consoleNewRow();
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
		case '\b':
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

		case '\t':
			currentConsole->cursorX  += currentConsole->tabSize - ((currentConsole->cursorX)%(currentConsole->tabSize));
			break;
		case '\n':
			consoleNewRow();
		case '\r':
			currentConsole->cursorX  = 0;
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
	consoleCls ('2');
}

//---------------------------------------------------------------------------------
void consoleSetWindow(PrintConsole* console, int x, int y, int width, int height) {
//---------------------------------------------------------------------------------

	if(!console) console = currentConsole;

	console->windowWidth = width;
	console->windowHeight = height;
	console->windowX = x;
	console->windowY = y;

	console->cursorX = 0;
	console->cursorY = 0;

}
