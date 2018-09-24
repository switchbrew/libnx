/**
 * @file console.h
 * @brief Framebuffer text console.
 * @author yellows8
 * @author WinterMute
 * @copyright libnx Authors
 *
 * Provides stdio integration for printing to the Switch screen as well as debug print
 * functionality provided by stderr.
 *
 * General usage is to initialize the console by:
 * @code
 * consoleInit(NULL)
 * @endcode
 * optionally customizing the console usage by passing a pointer to a custom PrintConsole struct.
 */
#pragma once
#include "../../types.h"

#define CONSOLE_ESC(x) "\x1b[" #x
#define CONSOLE_RESET   CONSOLE_ESC(0m)
#define CONSOLE_BLACK   CONSOLE_ESC(30m)
#define CONSOLE_RED     CONSOLE_ESC(31;1m)
#define CONSOLE_GREEN   CONSOLE_ESC(32;1m)
#define CONSOLE_YELLOW  CONSOLE_ESC(33;1m)
#define CONSOLE_BLUE    CONSOLE_ESC(34;1m)
#define CONSOLE_MAGENTA CONSOLE_ESC(35;1m)
#define CONSOLE_CYAN    CONSOLE_ESC(36;1m)
#define CONSOLE_WHITE   CONSOLE_ESC(37;1m)

// Forward declaration
typedef struct PrintConsole PrintConsole;

/// Renderer interface for the console.
typedef struct ConsoleRenderer
{
	bool (*init)(PrintConsole* con);
	void (*deinit)(PrintConsole* con);
	void (*drawChar)(PrintConsole* con, int x, int y, int c);
	void (*scrollWindow)(PrintConsole* con);
	void (*flushAndSwap)(PrintConsole* con);
} ConsoleRenderer;

/// A font struct for the console.
typedef struct ConsoleFont
{
	const void* gfx;         ///< A pointer to the font graphics
	u16 asciiOffset; ///< Offset to the first valid character in the font table
	u16 numChars;    ///< Number of characters in the font graphics
	u16 tileWidth;
	u16 tileHeight;
}ConsoleFont;

/**
 * @brief Console structure used to store the state of a console render context.
 *
 * Default values from consoleGetDefault();
 * @code
 * PrintConsole defaultConsole =
 * {
 * 	//Font:
 * 	{
 * 		default_font_bin, //font gfx
 * 		0, //first ascii character in the set
 * 		256, //number of characters in the font set
 * 		16, //tile width
 * 		16, //tile height
 *	},
 *	NULL, //renderer
 *	0,0, //cursorX cursorY
 *	0,0, //prevcursorX prevcursorY
 *	80, //console width
 *	45, //console height
 *	0,  //window x
 *	0,  //window y
 *	80, //window width
 *	45, //window height
 *	3, //tab size
 *	7, // foreground color
 *	0, // background color
 *	0, // flags
 *	false //console initialized
 * };
 * @endcode
 */
struct PrintConsole
{
	ConsoleFont font;        ///< Font of the console
	ConsoleRenderer* renderer; ///< Renderer of the console

	int cursorX;             ///< Current X location of the cursor (as a tile offset by default)
	int cursorY;             ///< Current Y location of the cursor (as a tile offset by default)

	int prevCursorX;         ///< Internal state
	int prevCursorY;         ///< Internal state

	int consoleWidth;        ///< Width of the console hardware layer in characters
	int consoleHeight;       ///< Height of the console hardware layer in characters

	int windowX;             ///< Window X location in characters
	int windowY;             ///< Window Y location in characters
	int windowWidth;         ///< Window width in characters
	int windowHeight;        ///< Window height in characters

	int tabSize;             ///< Size of a tab
	int fg;                  ///< Foreground color
	int bg;                  ///< Background color
	int flags;               ///< Reverse/bright flags

	bool consoleInitialised; ///< True if the console is initialized
};

#define CONSOLE_COLOR_BOLD	(1<<0) ///< Bold text
#define CONSOLE_COLOR_FAINT	(1<<1) ///< Faint text
#define CONSOLE_ITALIC		(1<<2) ///< Italic text
#define CONSOLE_UNDERLINE	(1<<3) ///< Underlined text
#define CONSOLE_BLINK_SLOW	(1<<4) ///< Slow blinking text
#define CONSOLE_BLINK_FAST	(1<<5) ///< Fast blinking text
#define CONSOLE_COLOR_REVERSE	(1<<6) ///< Reversed color text
#define CONSOLE_CONCEAL		(1<<7) ///< Concealed text
#define CONSOLE_CROSSED_OUT	(1<<8) ///< Crossed out text

/// Console debug devices supported by libnx.
typedef enum {
	debugDevice_NULL,    ///< Swallows prints to stderr
	debugDevice_SVC,     ///< Outputs stderr debug statements using svcOutputDebugString, which can then be captured by interactive debuggers
	debugDevice_CONSOLE, ///< Directs stderr debug statements to Switch console window
	debugDevice_3DMOO = debugDevice_SVC,
} debugDevice;

/**
 * @brief Loads the font into the console.
 * @param console Pointer to the console to update, if NULL it will update the current console.
 * @param font The font to load.
 */
void consoleSetFont(PrintConsole* console, ConsoleFont* font);

/**
 * @brief Sets the print window.
 * @param console Console to set, if NULL it will set the current console window.
 * @param x X location of the window.
 * @param y Y location of the window.
 * @param width Width of the window.
 * @param height Height of the window.
 */
void consoleSetWindow(PrintConsole* console, int x, int y, int width, int height);

/**
 * @brief Gets a pointer to the console with the default values.
 * This should only be used when using a single console or without changing the console that is returned, otherwise use consoleInit().
 * @return A pointer to the console with the default values.
 */
PrintConsole* consoleGetDefault(void);

/**
 * @brief Make the specified console the render target.
 * @param console A pointer to the console struct (must have been initialized with consoleInit(PrintConsole* console)).
 * @return A pointer to the previous console.
 */
PrintConsole *consoleSelect(PrintConsole* console);

/**
 * @brief Initialise the console.
 * @param console A pointer to the console data to initialize (if it's NULL, the default console will be used).
 * @return A pointer to the current console.
 */
PrintConsole* consoleInit(PrintConsole* console);

/**
 * @brief Deinitialise the console.
 * @param console A pointer to the console data to initialize (if it's NULL, the default console will be used).
 */
void consoleExit(PrintConsole* console);

/**
 * @brief Updates the console, submitting a new frame to the display.
 * @param console A pointer to the console data to initialize (if it's NULL, the default console will be used).
 * @remark This function should be called periodically. Failure to call this function will result in lack of screen updating.
 */
void consoleUpdate(PrintConsole* console);

/**
 * @brief Initializes debug console output on stderr to the specified device.
 * @param device The debug device (or devices) to output debug print statements to.
 */
void consoleDebugInit(debugDevice device);

/// Clears the screan by using printf("\x1b[2J");
void consoleClear(void);
