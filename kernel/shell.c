#include "shell.h"
#include "vga.h"
#include "string.h"

#define INPUT_BUFFER_SIZE 256

static char input_buffer[INPUT_BUFFER_SIZE];
static int input_index = 0;

#define HISTORY_SIZE 10
static char history[HISTORY_SIZE][INPUT_BUFFER_SIZE];
static int history_count = 0;
static int history_index = -1;
static int cursor_pos = 0;

// Forward declamations form via.c
void terminal_writestring(const char* data);
void terminal_putchar(char c);
void terminal_backspace(void);
void terminal_setcolor(uint8_t color);

// forward dec from vga.h
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);

// ++++++++++++++++++++
// + String Utilities +
// ++++++++++++++++++++

static int shell_strcmp(const char* a, const char* b)
{
	while (*a && *b && *a == *b) { a++; b++; }
	return *a - *b;
}

static int shell_strncmp(const char* a, const char* b, size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		if (a[i] != b[i]) return a[i] - b[i];
		if (a[i] == '\0') return 0;
	}
	return 0;
}

//++++++++++++
//+ Commands +
//++++++++++++

// help
static void cmd_help(void)
{
	terminal_writestring("Available Commands\n");
	terminal_writestring("	help	- Shows this message\n");
	terminal_writestring("	clear	- Clears screen\n");
	terminal_writestring("	echo	- Print text to screen\n");
	terminal_writestring("	about	- About GordOS\n");
}

// clear
static void cmd_clear(void)
{
	// Clear by writing spaces to every cell
	for (int y = 0; y < 25; y++)
		for (int x = 0; x < 80; x++)
			terminal_putchar(' ');

	// reset cursor to  top by reinitializing terminal
	extern void terminal_initialize(void);
	terminal_initialize();
}

//echo
static void cmd_echo(const char* args)
{
	if (args == 0 || *args == '\0')
	{
		terminal_putchar('\n');
		return;
	}

	terminal_writestring(args);
	terminal_putchar('\n');
}

// About
static void cmd_about(void)
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
	terminal_writestring("	GordOS\n");
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	terminal_writestring("	A hobby OS by Hamish Gordon\n");
	terminal_writestring("	Built from scratch in C and x86 Assembly\n");
	terminal_writestring("	https://github.io/Millenium2133/GordOS\n");
}

// ++++++++++++++++++++
// + Command Dispatch +
// ++++++++++++++++++++

//simple Tokenizer
static const char* get_args(const char* input, size_t cmd_len)
{
	if (input[cmd_len] == ' ' && input[cmd_len + 1] != '\0')
		return &input[cmd_len + 1];
	return 0;
}

static void shell_execute(const char* input)
{
	if (*input == '\0')
		return;

	if (shell_strcmp(input, "help") == 0)
		cmd_help();
	else if (shell_strcmp(input, "clear") == 0)
		cmd_clear();
	else if (shell_strncmp(input, "echo", 4) == 0)
		cmd_echo(get_args(input, 4));
	else if (shell_strcmp(input, "about") == 0)
		cmd_about();
	else
	{
		terminal_writestring("Unknown Command: ");
		terminal_writestring(input);
		terminal_putchar('\n');
	}
}

// ++++++++++++++++++
// + Input Handling +
// ++++++++++++++++++

static void shell_prompt(void)
{
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
	terminal_writestring("GordOS");
	terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
	terminal_writestring("> ");
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

void shell_handle_char(char c)
{
    if (c == '\n')
    {
        terminal_putchar('\n');
        input_buffer[input_index] = '\0';

        // Save to history if non-empty
        if (input_index > 0)
        {
            // Shift history up if full
            if (history_count == HISTORY_SIZE)
            {
                for (int i = 0; i < HISTORY_SIZE - 1; i++)
                {
                    for (int j = 0; j < INPUT_BUFFER_SIZE; j++)
                        history[i][j] = history[i + 1][j];
                }
                history_count--;
            }
            // Copy current input into history
            for (int i = 0; i <= input_index; i++)
                history[history_count][i] = input_buffer[i];
            history_count++;
        }

        shell_execute(input_buffer);
        input_index = 0;
        cursor_pos = 0;
        history_index = -1;
        shell_prompt();
    }
    else if (c == '\b')
    {
        // Backspace at cursor position
        if (cursor_pos > 0)
        {
            // Shift everything left from cursor
            for (int i = cursor_pos - 1; i < input_index - 1; i++)
                input_buffer[i] = input_buffer[i + 1];
            input_index--;
            cursor_pos--;

            // Redraw from cursor to end, then erase last char
            for (int i = cursor_pos; i < input_index; i++)
                terminal_putchar(input_buffer[i]);
            terminal_putchar(' ');  // erase the last character

            // Move cursor back to correct position
            for (int i = cursor_pos; i < input_index + 1; i++)
                terminal_backspace();
        }
    }
    else if ((unsigned char)c == KEY_LEFT)
    {
        if (cursor_pos > 0)
        {
            cursor_pos--;
            terminal_backspace();
        }
    }
    else if ((unsigned char)c == KEY_RIGHT)
    {
        if (cursor_pos < input_index)
        {
            terminal_putchar(input_buffer[cursor_pos]);
            cursor_pos++;
        }
    }
    else if ((unsigned char)c == KEY_UP)
    {
        // Go back in history
        int new_index = history_index + 1;
        if (new_index < history_count)
        {
            history_index = new_index;
            // Clear current input line
            for (int i = 0; i < input_index; i++)
                terminal_backspace();
            // Load history entry (newest first)
            int entry = history_count - 1 - history_index;
            input_index = 0;
            cursor_pos = 0;
            for (int i = 0; history[entry][i] != '\0'; i++)
            {
                input_buffer[input_index++] = history[entry][i];
                terminal_putchar(history[entry][i]);
                cursor_pos++;
            }
        }
    }
    else if ((unsigned char)c == KEY_DOWN)
    {
        // Go forward in history
        history_index--;
        if (history_index < 0)
        {
            // Back to empty prompt
            history_index = -1;
            for (int i = 0; i < input_index; i++)
                terminal_backspace();
            input_index = 0;
            cursor_pos = 0;
        }
        else
        {
            // Load history entry
            for (int i = 0; i < input_index; i++)
                terminal_backspace();
            int entry = history_count - 1 - history_index;
            input_index = 0;
            cursor_pos = 0;
            for (int i = 0; history[entry][i] != '\0'; i++)
            {
                input_buffer[input_index++] = history[entry][i];
                terminal_putchar(history[entry][i]);
                cursor_pos++;
            }
        }
    }
    else if (input_index < INPUT_BUFFER_SIZE - 1)
    {
        // Insert character at cursor position
        // Shift everything right from cursor
        for (int i = input_index; i > cursor_pos; i--)
            input_buffer[i] = input_buffer[i - 1];
        input_buffer[cursor_pos] = c;
        input_index++;

        // Redraw from cursor to end
        for (int i = cursor_pos; i < input_index; i++)
            terminal_putchar(input_buffer[i]);
        cursor_pos++;

        // Move cursor back to correct position
        for (int i = cursor_pos; i < input_index; i++)
            terminal_backspace();
    }
}
 
void shell_init(void)
{
	input_index = 0;
	cursor_pos = 0;
	history_index = -1;
	shell_prompt();
}



















