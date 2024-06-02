#include "embedded_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_TOKEN_NPOS 0xffff

// Terminal colors
#define BLACK "30"
#define RED "31"
#define GREEN "32"
#define YELLOW "33"
#define BLUE "34"
#define MAGENTA "35"
#define CYAN "36"
#define WHITE "37"
#define GRAY "90"
// Terminal background colors
#define BLACK_BK "40"
#define RED_BK "41"
#define GREEN_BK "42"
#define YELLOW_BK "43"
#define BLUE_BK "44"
#define MAGENTA_BK "45"
#define CYAN_BK "46"
#define WHITE_BK "47"
#define GRAY_BK "100"
// Terminal format codes
#define RESET "0"
#define BOLD "1"
#define LIGHT "2"
#define ITALIC "3"
#define UNDERLINE "4"
#define BLINK "5"
#define REVERSE "7"
#define HIDE "8"
#define CROSS "9"
// Terminal format functions
#define FMT1(fmt1) "\033[" fmt1 "m"
#define FMT2(fmt1, fmt2) "\033[" fmt1 ";" fmt2 "m"
#define FMT3(fmt1, fmt2, fmt3) "\033[" fmt1 ";" fmt2 ";" fmt3 "m"
#define FMT4(fmt1, fmt2, fmt3, fmt4) "\033[" fmt1 ";" fmt2 ";" fmt3 ";" fmt4 "m"
#define FMT_RESET FMT1(RESET)

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#define PREPARE_IMPL(t) EmbeddedCliImpl* impl = (EmbeddedCliImpl*)t->_impl

#define IS_FLAG_SET(flags, flag) (((flags) & (flag)) != 0)

#define SET_FLAG(flags, flag) ((flags) |= (flag))

#define UNSET_U8FLAG(flags, flag) ((flags) &= (uint8_t) ~(flag))

const static char* CLI_INVITATION_COLOR = FMT2(BOLD, BLUE);

const static char* CLI_SUB_INTERPRETER_INVITATION_COLOR = FMT2(BOLD, MAGENTA);

const static char* CLI_AUTOCOMPLETE_COLOR = FMT1(GRAY);

const static char* CLI_HELP_HEADER_COLOR = FMT2(BOLD, GREEN);

const static char* CLI_HELP_CONTENT_COLOR = FMT1(GREEN);

const static char* CLI_WARNING_COLOR = FMT2(BOLD, YELLOW);

const static char* CLI_ERROR_COLOR = FMT2(BOLD, RED);

/**
 * Marks binding as candidate for autocompletion
 * This flag is updated each time getAutocompletedCommand is called
 */
#define BINDING_FLAG_AUTOCOMPLETE 1u

/**
 * Indicates that rx buffer overflow happened. In such case last command
 * that wasn't finished (no \r or \n were received) will be discarded
 */
#define CLI_FLAG_OVERFLOW 0x01u

/**
 * Indicates that initialization is completed. Initialization is completed in
 * first call to process and needed, for example, to print invitation message.
 */
#define CLI_FLAG_INIT_COMPLETE 0x02u

/**
 * Indicates that CLI structure and internal structures were allocated with
 * malloc and should be freed
 */
#define CLI_FLAG_ALLOCATED 0x04u

/**
 * Indicates that escape mode is enabled.
 */
#define CLI_FLAG_ESCAPE_MODE 0x08u

/**
 * Indicates that CLI in mode when it will print directly to output without
 * clear of current command and printing it back
 */
#define CLI_FLAG_DIRECT_PRINT 0x10u

/**
 * Indicates that live autocompletion is enabled
 */
#define CLI_FLAG_AUTOCOMPLETE_ENABLED 0x20u

/**
 * Indicates that color output is enabled
 */
#define CLI_FLAG_COLOR_OUTPUT_ENABLED 0x40u

/**
 * Indicates that sub interpreter mode is enabled
 */
#define CLI_FLAG_SUB_INTERPRETER_ENABLED 0x80u

/**
 * Indicates that cursor direction should be forward
 */
#define CURSOR_DIRECTION_FORWARD true

/**
 * Indicates that cursor direction should be backward
 */
#define CURSOR_DIRECTION_BACKWARD false

typedef struct EmbeddedCliImpl EmbeddedCliImpl;
typedef struct AutocompletedCommand AutocompletedCommand;
typedef struct FifoBuf FifoBuf;
typedef struct CliHistory CliHistory;

struct FifoBuf {
    char* buf;
    /**
   * Position of first element in buffer. From this position elements are taken
   */
    uint16_t front;
    /**
   * Position after last element. At this position new elements are inserted
   */
    uint16_t back;
    /**
   * Size of buffer
   */
    uint16_t size;
};

struct CliHistory {
    /**
   * Items in buffer are separated by null-chars
   */
    char* buf;

    /**
   * Total size of buffer
   */
    uint16_t bufferSize;

    /**
   * Index of currently selected element. This allows to navigate history
   * After command is sent, current element is reset to 0 (no element)
   */
    uint16_t current;

    /**
   * Number of items in buffer
   * Items are counted from top to bottom (and are 1 based).
   * So the most recent item is 1 and the oldest is itemCount.
   */
    uint16_t itemsCount;
};

struct EmbeddedCliImpl {
    /**
   * Invitation string. Is printed at the beginning of each line with user
   * input
   */
    const char* invitation;

    CliHistory history;

    /**
   * Buffer for storing received chars.
   * Chars are stored in FIFO mode.
   */
    FifoBuf rxBuffer;

    /**
   * Buffer for current command
   */
    char* cmdBuffer;

    /**
   * Size of current command
   */
    uint16_t cmdSize;

    /**
   * Total size of command buffer
   */
    uint16_t cmdMaxSize;

    CliCommandBinding* bindings;

    /**
   * Flags for each binding. Sizes are the same as for bindings array
   */
    uint8_t* bindingsFlags;

    uint16_t bindingsCount;

    uint16_t maxBindingsCount;

    uint16_t currentBinding;

    /**
   * Total length of input line. This doesn't include invitation but
   * includes current command and its live autocompletion
   */
    uint16_t inputLineLength;

    /**
   * Stores last character that was processed.
   */
    char lastChar;

    /**
   * Flags are defined as CLI_FLAG_*
   */
    uint8_t flags;

    /**
   * Cursor position for current command from right to left
   * 0 = end of command
   */
    uint16_t cursorPos;

    /**
   * Pointer to a sub handler to handle raw data inputted to cli
   */
    char (*rawHandler)(EmbeddedCli* cli, char data);

    /**
   * Pointer to a sub handler to handle raw data inputted to cli
   */
    void (*rawBufferHandler)(EmbeddedCli* cli, const char* buffer, size_t len);

    /**
   * Pointer to a sub interpreter function. If set, this function will
   * handle all commands inputted to cli.
   */
    void (*subInterpreterOnCmd)(EmbeddedCli* cli, CliCommand* command);

    /**
   * Pointer to a function that is called when sub interpreter is exited
   * Used to clean up resources, etc.
   */
    void (*subInterpreterOnExit)(EmbeddedCli* cli);

    /**
   * Invitation string for sub interpreter. Is printed at the beginning of
   * each line with user input
   */
    const char* subInterpreterInvitation;
};

struct AutocompletedCommand {
    /**
   * Name of autocompleted command (or first candidate for autocompletion if
   * there are multiple candidates).
   * NULL if autocomplete not possible.
   */
    const char* firstCandidate;

    /**
   * Number of characters that can be completed safely. For example, if there
   * are two possible commands "get-led" and "get-adc", then for prefix "g"
   * autocompletedLen will be 4. If there are only one candidate, this number
   * is always equal to length of the command.
   */
    uint16_t autocompletedLen;

    /**
   * Total number of candidates for autocompletion
   */
    uint16_t candidateCount;
};

static EmbeddedCliConfig defaultConfig;

/**
 * Number of commands that cli adds. Commands:
 * - help
 * - clear
 */
static const uint16_t cliInternalBindingCount = 3;

static const char* lineBreak = "\r\n";

/* References for VT100 escape sequences:
 * https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
 * https://ecma-international.org/publications-and-standards/standards/ecma-48/
 */

/** Escape sequence - Cursor forward (right) */
static const char* escSeqCursorRight = "\x1B[C";

/** Escape sequence - Cursor backward (left) */
static const char* escSeqCursorLeft = "\x1B[D";

/** Escape sequence - Cursor save position */
static const char* escSeqCursorSave = "\x1B[s";

/** Escape sequence - Cursor restore position */
static const char* escSeqCursorRestore = "\x1B[u";

/** Escape sequence - Cursor insert character (ICH) */
static const char* escSeqInsertChar = "\x1B[@";

/** Escape sequence - Cursor delete character (DCH) */
static const char* escSeqDeleteChar = "\x1B[P";

/**
 * Navigate through command history back and forth. If navigateUp is true,
 * navigate to older commands, otherwise navigate to newer.
 * When history end is reached, nothing happens.
 * @param cli
 * @param navigateUp
 */
static void navigateHistory(EmbeddedCli* cli, bool navigateUp);

/**
 * Process escaped character. After receiving ESC+[ sequence, all chars up to
 * ending character are sent to this function
 * @param cli
 * @param c
 */
static void onEscapedInput(EmbeddedCli* cli, char c);

/**
 * Process input character. Character is valid displayable char and should be
 * added to current command string and displayed to client.
 * @param cli
 * @param c
 */
static void onCharInput(EmbeddedCli* cli, char c);

/**
 * Process control character (like \r or \n) possibly altering state of current
 * command or executing onCommand callback.
 * @param cli
 * @param c
 */
static void onControlInput(EmbeddedCli* cli, char c);

/**
 * Parse command in buffer and execute callback
 * @param cli
 */
static void parseCommand(EmbeddedCli* cli);

/**
 * Print invitation to cli output
 * @param cli
 */
static void printInvitation(EmbeddedCli* cli);

/**
 * Print help for given binding (if it is set)
 * @param binding
 */
static void printBindingHelp(EmbeddedCli* cli, CliCommandBinding* binding);

/**
 * Print usage for given binding (if it is set)
 * @param binding
 */
static void printBindingUsage(EmbeddedCli* cli, CliCommandBinding* binding);

/**
 * Setup bindings for internal commands, like help
 * @param cli
 */
static void initInternalBindings(EmbeddedCli* cli);

/**
 * Show help for given tokens (or default help if no tokens)
 * @param cli
 * @param tokens
 * @param context - not used
 */
static void onHelp(EmbeddedCli* cli, char* tokens, void* context);

/**
 * Clear terminal screen
 * @param cli
 * @param tokens - not used
 * @param context - not used
 */
static void onClear(EmbeddedCli* cli, char* tokens, void* context);

#ifdef __CORTEX_M
/**
 * Reboot device (only for Cortex-M devices)
 * @param cli
 * @param tokens - not used
 * @param context - not used
 */
static void onReboot(EmbeddedCli* cli, char* tokens, void* context);
#endif

/**
 * Show error about unknown command
 * @param cli
 * @param name
 */
static void onUnknownCommand(EmbeddedCli* cli, const char* name);

/**
 * Return autocompleted command for given prefix.
 * Prefix is compared to all known command bindings and autocompleted result
 * is returned
 * @param cli
 * @param prefix
 * @return
 */
static AutocompletedCommand getAutocompletedCommand(EmbeddedCli* cli,
                                                    const char* prefix);

/**
 * Prints autocompletion result while keeping current command unchanged
 * Prints only if autocompletion is present and only one candidate exists.
 * @param cli
 */
static void printLiveAutocompletion(EmbeddedCli* cli);

/**
 * Handles autocomplete request. If autocomplete possible - fills current
 * command with autocompleted command. When multiple commands satisfy entered
 * prefix, they are printed to output.
 * @param cli
 * @param printCandidates - if true, all candidates are printed to output
 */
static void onAutocompleteRequest(EmbeddedCli* cli, bool printCandidates);

/**
 * Removes all input from current line (replaces it with whitespaces)
 * And places cursor at the beginning of the line
 * @param cli
 */
static void clearCurrentLine(EmbeddedCli* cli);

/**
 * Write given string to cli output
 * @param cli
 * @param str
 */
static void writeToOutput(EmbeddedCli* cli, const char* str);

/**
 * Write given string to cli output with color if color output is enabled
 * @param cli
 * @param str - string to write, use FMT* macros to format it
 */
static void writeToOutputColor(EmbeddedCli* cli, const char* str,
                               const char* fmt_string);

/**
 * Move cursor forward (right) by given number of positions
 * @param cli
 * @param count
 * @param direction: true = forward (right), false = backward (left)
 */
static void moveCursor(EmbeddedCli* cli, uint16_t count, bool direction);

/**
 * Returns true if provided char is a supported control char:
 * \r, \n, \b or 0x7F (treated as \b)
 * @param c
 * @return
 */
static bool isControlChar(char c);

/**
 * Returns true if provided char is a valid displayable character:
 * a-z, A-Z, 0-9, whitespace, punctuation, etc.
 * Currently only ASCII is supported
 * @param c
 * @return
 */
static bool isDisplayableChar(char c);

/**
 * How many elements are currently available in buffer
 * @param buffer
 * @return number of elements
 */
static uint16_t fifoBufAvailable(FifoBuf* buffer);

/**
 * Return first character from buffer and remove it from buffer
 * Buffer must be non-empty, otherwise 0 is returned
 * @param buffer
 * @return
 */
static char fifoBufPop(FifoBuf* buffer);

/**
 * Push character into fifo buffer. If there is no space left, character is
 * discarded and false is returned
 * @param buffer
 * @param a - character to add
 * @return true if char was added to buffer, false otherwise
 */
static bool fifoBufPush(FifoBuf* buffer, char a);

/**
 * Copy provided string to the history buffer.
 * If it is already inside history, it will be removed from it and added again.
 * So after addition, it will always be on top
 * If available size is not enough (and total size is enough) old elements will
 * be removed from history so this item can be put to it
 * @param history
 * @param str
 * @return true if string was put in history
 */
static bool historyPut(CliHistory* history, const char* str);

/**
 * Get item from history. Items are counted from 1 so if item is 0 or greater
 * than itemCount, NULL is returned
 * @param history
 * @param item
 * @return true if string was put in history
 */
static const char* historyGet(CliHistory* history, uint16_t item);

/**
 * Remove specific item from history
 * @param history
 * @param str - string to remove
 * @return
 */
static void historyRemove(CliHistory* history, const char* str);

/**
 * Return position (index of first char) of specified token
 * @param tokenizedStr - tokenized string (separated by \0 with
 * \0\0 at the end)
 * @param pos - token position (counted from 1)
 * @return index of first char of specified token
 */
static uint16_t getTokenPosition(const char* tokenizedStr, uint16_t pos);
static uint16_t getPositivePos(const char* tokenizedStr, int16_t pos);

EmbeddedCliConfig* embeddedCliDefaultConfig(void) {
    defaultConfig.rxBufferSize = 64;
    defaultConfig.cmdBufferSize = 64;
    defaultConfig.historyBufferSize = 128;
    defaultConfig.cliBuffer = NULL;
    defaultConfig.cliBufferSize = 0;
    defaultConfig.maxBindingCount = 32;
    defaultConfig.enableAutoComplete = true;
    defaultConfig.enableColorOutput = true;
    defaultConfig.invitation = "CLI > ";
    return &defaultConfig;
}

uint16_t embeddedCliRequiredSize(EmbeddedCliConfig* config) {
    uint16_t bindingCount =
        (uint16_t)(config->maxBindingCount + cliInternalBindingCount);
    return (
        uint16_t)(CLI_UINT_SIZE *
                  (BYTES_TO_CLI_UINTS(sizeof(EmbeddedCli)) +
                   BYTES_TO_CLI_UINTS(sizeof(EmbeddedCliImpl)) +
                   BYTES_TO_CLI_UINTS(config->rxBufferSize * sizeof(char)) +
                   BYTES_TO_CLI_UINTS(config->cmdBufferSize * sizeof(char)) +
                   BYTES_TO_CLI_UINTS(config->historyBufferSize *
                                      sizeof(char)) +
                   BYTES_TO_CLI_UINTS(bindingCount *
                                      sizeof(CliCommandBinding)) +
                   BYTES_TO_CLI_UINTS(bindingCount * sizeof(uint8_t))));
}

EmbeddedCli* embeddedCliNew(EmbeddedCliConfig* config) {
    EmbeddedCli* cli = NULL;

    uint16_t bindingCount =
        (uint16_t)(config->maxBindingCount + cliInternalBindingCount);

    size_t totalSize = embeddedCliRequiredSize(config);

    bool allocated = false;
    if (config->cliBuffer == NULL) {
        config->cliBuffer = m_alloc(totalSize);
        if (config->cliBuffer == NULL)
            return NULL;
        allocated = true;
    } else if (config->cliBufferSize < totalSize) {
        return NULL;
    }

    CLI_UINT* buf = config->cliBuffer;

    memset(buf, 0, totalSize);

    cli = (EmbeddedCli*)buf;
    buf += BYTES_TO_CLI_UINTS(sizeof(EmbeddedCli));

    cli->_impl = (EmbeddedCliImpl*)buf;
    buf += BYTES_TO_CLI_UINTS(sizeof(EmbeddedCliImpl));

    PREPARE_IMPL(cli);
    impl->rxBuffer.buf = (char*)buf;
    buf += BYTES_TO_CLI_UINTS(config->rxBufferSize * sizeof(char));

    impl->cmdBuffer = (char*)buf;
    buf += BYTES_TO_CLI_UINTS(config->cmdBufferSize * sizeof(char));

    impl->bindings = (CliCommandBinding*)buf;
    buf += BYTES_TO_CLI_UINTS(bindingCount * sizeof(CliCommandBinding));

    impl->bindingsFlags = (uint8_t*)buf;
    buf += BYTES_TO_CLI_UINTS(bindingCount);

    impl->history.buf = (char*)buf;
    impl->history.bufferSize = config->historyBufferSize;

    if (allocated)
        SET_FLAG(impl->flags, CLI_FLAG_ALLOCATED);

    if (config->enableAutoComplete)
        SET_FLAG(impl->flags, CLI_FLAG_AUTOCOMPLETE_ENABLED);

    if (config->enableColorOutput)
        SET_FLAG(impl->flags, CLI_FLAG_COLOR_OUTPUT_ENABLED);

    impl->rxBuffer.size = config->rxBufferSize;
    impl->rxBuffer.front = 0;
    impl->rxBuffer.back = 0;
    impl->cmdMaxSize = config->cmdBufferSize;
    impl->bindingsCount = 0;
    impl->maxBindingsCount =
        (uint16_t)(config->maxBindingCount + cliInternalBindingCount);
    impl->lastChar = '\0';
    impl->invitation = config->invitation;
    impl->cursorPos = 0;
    impl->subInterpreterOnCmd = NULL;
    impl->subInterpreterInvitation = NULL;
    impl->subInterpreterOnExit = NULL;
    impl->rawHandler = NULL;
    impl->rawBufferHandler = NULL;

    initInternalBindings(cli);

    return cli;
}

static void defaultWriteChar(EmbeddedCli* embeddedCli, char c) {
    putchar(c);
}

EmbeddedCli* embeddedCliNewDefault(void) {
    EmbeddedCli* cli = embeddedCliNew(embeddedCliDefaultConfig());
    if (cli != NULL) {
        cli->writeChar = defaultWriteChar;
    }
    return cli;
}

void embeddedCliReceiveChar(EmbeddedCli* cli, char c) {
    if (!cli)
        return;
    PREPARE_IMPL(cli);
    if (impl->rawBufferHandler) {
        impl->rawBufferHandler(cli, &c, 1);
        return;
    }
    if (!fifoBufPush(&impl->rxBuffer, c)) {
        SET_FLAG(impl->flags, CLI_FLAG_OVERFLOW);
    }
}

void embeddedCliReceiveBuffer(EmbeddedCli* cli, const char* buffer,
                              size_t len) {
    if (!cli || !buffer)
        return;
    PREPARE_IMPL(cli);
    if (impl->rawBufferHandler) {
        impl->rawBufferHandler(cli, buffer, len);
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!fifoBufPush(&impl->rxBuffer, buffer[i])) {
            SET_FLAG(impl->flags, CLI_FLAG_OVERFLOW);
            break;
        }
    }
}

void embeddedCliSetInvitation(EmbeddedCli* cli, const char* invitation) {
    PREPARE_IMPL(cli);
    impl->invitation = invitation;
}

void embeddedCliSetRawHandler(EmbeddedCli* cli,
                              char (*rawHandler)(EmbeddedCli* cli, char data)) {
    PREPARE_IMPL(cli);
    impl->rawHandler = rawHandler;
    UNSET_U8FLAG(impl->flags, CLI_FLAG_INIT_COMPLETE);
}

void embeddedCliResetRawHandler(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    impl->rawHandler = NULL;
}

void embeddedCliSetRawBufferHandler(EmbeddedCli* cli,
                                    void (*rawBufferHandler)(EmbeddedCli* cli,
                                                             const char* buffer,
                                                             size_t len)) {
    PREPARE_IMPL(cli);
    impl->rawBufferHandler = rawBufferHandler;
    UNSET_U8FLAG(impl->flags, CLI_FLAG_INIT_COMPLETE);
}

void embeddedCliResetRawBufferHandler(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    impl->rawBufferHandler = NULL;
}

static void printInvitation(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    if (!IS_FLAG_SET(impl->flags, CLI_FLAG_INIT_COMPLETE)) {
        return;  // no invitation before initialization is complete
    }
    if (!IS_FLAG_SET(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED)) {
        writeToOutputColor(cli, impl->invitation, CLI_INVITATION_COLOR);
    } else {
        if (impl->subInterpreterInvitation != NULL) {
            writeToOutputColor(cli, impl->subInterpreterInvitation,
                               CLI_SUB_INTERPRETER_INVITATION_COLOR);
        } else {
            writeToOutputColor(cli, "> ", CLI_SUB_INTERPRETER_INVITATION_COLOR);
        }
    }
}

void embeddedCliEnterSubInterpreter(
    EmbeddedCli* cli, void (*onCommand)(EmbeddedCli* cli, CliCommand* command),
    void (*onExit)(EmbeddedCli* cli), const char* invitation) {
    PREPARE_IMPL(cli);
    impl->subInterpreterOnCmd = onCommand;
    impl->subInterpreterOnExit = onExit;
    impl->subInterpreterInvitation = invitation;
    SET_FLAG(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED);
    // clear history
    impl->history.current = 0;
    impl->history.itemsCount = 0;
}

void embeddedCliExitSubInterpreter(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    if (impl->subInterpreterOnExit != NULL)
        impl->subInterpreterOnExit(cli);
    impl->subInterpreterOnCmd = NULL;
    impl->subInterpreterInvitation = NULL;
    impl->subInterpreterOnExit = NULL;
    UNSET_U8FLAG(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED);
    // clear history
    impl->history.current = 0;
    impl->history.itemsCount = 0;
}

void embeddedCliProcess(EmbeddedCli* cli) {
    if (cli->writeChar == NULL)
        return;

    PREPARE_IMPL(cli);

    if (impl->rawHandler != NULL) {
        // if sub handler is set, it will handle all input
        while (fifoBufAvailable(&impl->rxBuffer)) {
            char c = fifoBufPop(&impl->rxBuffer);
            c = impl->rawHandler(cli, c);
            if (c != '\0') {
                if (c == '\r' || c == '\n') {
                    writeToOutput(cli, lineBreak);
                } else if (c >= 32 && c <= 126) {
                    cli->writeChar(cli, c);
                }
            }
        }
        return;
    }

    if (impl->rawBufferHandler != NULL) {
        // if sub handler is set, it will handle all input
        return;
    }

    if (!IS_FLAG_SET(impl->flags, CLI_FLAG_INIT_COMPLETE)) {
        SET_FLAG(impl->flags, CLI_FLAG_INIT_COMPLETE);
        printInvitation(cli);
    }

    while (fifoBufAvailable(&impl->rxBuffer)) {
        char c = fifoBufPop(&impl->rxBuffer);

        if (IS_FLAG_SET(impl->flags, CLI_FLAG_ESCAPE_MODE)) {
            onEscapedInput(cli, c);
        } else if (impl->lastChar == 0x1B && c == '[') {
            // enter escape mode
            SET_FLAG(impl->flags, CLI_FLAG_ESCAPE_MODE);
        } else if (isControlChar(c)) {
            onControlInput(cli, c);
        } else if (isDisplayableChar(c)) {
            onCharInput(cli, c);
        }

        printLiveAutocompletion(cli);

        impl->lastChar = c;
    }

    // discard unfinished command if overflow happened
    if (IS_FLAG_SET(impl->flags, CLI_FLAG_OVERFLOW)) {
        impl->cmdSize = 0;
        impl->cmdBuffer[impl->cmdSize] = '\0';
        UNSET_U8FLAG(impl->flags, CLI_FLAG_OVERFLOW);
    }
}

bool embeddedCliAddBinding(EmbeddedCli* cli, CliCommandBinding binding) {
    PREPARE_IMPL(cli);
    if (impl->bindingsCount == impl->maxBindingsCount)
        return false;

    impl->bindings[impl->bindingsCount] = binding;

    ++impl->bindingsCount;
    return true;
}

bool embeddedCliDelBinding(EmbeddedCli* cli, const char* name) {
    PREPARE_IMPL(cli);
    if (impl->bindingsCount == 0)
        return false;

    for (int i = 0; i < impl->bindingsCount; ++i) {
        if (strcmp(impl->bindings[i].name, name) == 0) {
            --impl->bindingsCount;
            if (impl->bindingsCount != 0 && i != impl->bindingsCount)
                impl->bindings[i] = impl->bindings[impl->bindingsCount];
            return true;
        }
    }

    return false;
}

void embeddedCliPrint(EmbeddedCli* cli, const char* string) {
    if (cli->writeChar == NULL)
        return;

    PREPARE_IMPL(cli);

    // Save cursor position
    uint16_t cursorPosSave = impl->cursorPos;
    // remove chars for autocompletion and live command
    if (!IS_FLAG_SET(impl->flags, CLI_FLAG_DIRECT_PRINT))
        clearCurrentLine(cli);
    // Restore cursor position
    impl->cursorPos = cursorPosSave;
    // print provided string
    writeToOutput(cli, string);
    writeToOutput(cli, lineBreak);

    // print current command back to screen
    if (!IS_FLAG_SET(impl->flags, CLI_FLAG_DIRECT_PRINT)) {
        printInvitation(cli);
        writeToOutput(cli, impl->cmdBuffer);
        impl->inputLineLength = impl->cmdSize;
        moveCursor(cli, impl->cursorPos, CURSOR_DIRECTION_BACKWARD);

        printLiveAutocompletion(cli);
    }
}

void embeddedCliFree(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    if (IS_FLAG_SET(impl->flags, CLI_FLAG_ALLOCATED)) {
        // allocation is done in single call to malloc, so need only single free
        // free(cli);
        m_free(cli);
    }
}

void embeddedCliTokenizeArgs(char* args) {
    if (args == NULL)
        return;

    // for now only space, but can add more later
    const char* separators = " ";

    // indicates that arg is quoted so separators are copied as is
    bool quotesEnabled = false;
    // indicates that previous char was a slash, so next char is copied as is
    bool escapeActivated = false;
    int insertPos = 0;

    int i = 0;
    char currentChar;
    while ((currentChar = args[i]) != '\0') {
        ++i;

        if (escapeActivated) {
            escapeActivated = false;
        } else if (currentChar == '\\') {
            escapeActivated = true;
            continue;
        } else if (currentChar == '"') {
            quotesEnabled = !quotesEnabled;
            currentChar = '\0';
        } else if (!quotesEnabled && strchr(separators, currentChar) != NULL) {
            currentChar = '\0';
        }

        // null chars are only copied once and not copied to the beginning
        if (currentChar != '\0' ||
            (insertPos > 0 && args[insertPos - 1] != '\0')) {
            args[insertPos] = currentChar;
            ++insertPos;
        }
    }

    // make args double null-terminated source buffer must be big enough to
    // contain extra spaces
    args[insertPos] = '\0';
    args[insertPos + 1] = '\0';
}

const char* embeddedCliGetToken(const char* tokenizedStr, int16_t pos) {
    uint16_t i =
        getTokenPosition(tokenizedStr, getPositivePos(tokenizedStr, pos));

    if (i != CLI_TOKEN_NPOS)
        return &tokenizedStr[i];
    else
        return NULL;
}

char* embeddedCliGetTokenVariable(char* tokenizedStr, int16_t pos) {
    uint16_t i =
        getTokenPosition(tokenizedStr, getPositivePos(tokenizedStr, pos));

    if (i != CLI_TOKEN_NPOS)
        return &tokenizedStr[i];
    else
        return NULL;
}

void embeddedCliPrintCurrentHelp(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    if (impl->currentBinding >= impl->bindingsCount)
        return;

    printBindingUsage(cli, &impl->bindings[impl->currentBinding]);
}

bool embeddedCliCheckToken(const char* tokenizedStr, const char* token,
                           int16_t pos) {
    return strcmp(embeddedCliGetToken(tokenizedStr,
                                      getPositivePos(tokenizedStr, pos)),
                  token) == 0;
}

bool embeddedCliCheckTokenStartswith(const char* tokenizedStr,
                                     const char* token, int16_t pos) {
    return strncmp(embeddedCliGetToken(tokenizedStr,
                                       getPositivePos(tokenizedStr, pos)),
                   token, strlen(token)) == 0;
}

bool embeddedCliCheckTokenEndswith(const char* tokenizedStr, const char* token,
                                   int16_t pos) {
    const char* t =
        embeddedCliGetToken(tokenizedStr, getPositivePos(tokenizedStr, pos));
    return strcmp(t + strlen(t) - strlen(token), token) == 0;
}

uint16_t embeddedCliFindToken(const char* tokenizedStr, const char* token) {
    if (tokenizedStr == NULL || token == NULL)
        return 0;

    uint16_t size = embeddedCliGetTokenCount(tokenizedStr);
    for (uint16_t i = 1; i <= size; ++i) {
        if (strcmp(embeddedCliGetToken(tokenizedStr, i), token) == 0)
            return i;
    }

    return 0;
}

uint16_t embeddedCliFindTokenStartswith(const char* tokenizedStr,
                                        const char* token) {
    if (tokenizedStr == NULL || token == NULL)
        return 0;
    uint16_t size = embeddedCliGetTokenCount(tokenizedStr);
    for (uint16_t i = 1; i <= size; ++i) {
        if (embeddedCliCheckTokenStartswith(tokenizedStr, token, i))
            return i;
    }
    return 0;
}

uint16_t embeddedCliFindTokenEndswith(const char* tokenizedStr,
                                      const char* token) {
    if (tokenizedStr == NULL || token == NULL)
        return 0;
    uint16_t size = embeddedCliGetTokenCount(tokenizedStr);
    for (uint16_t i = 1; i <= size; ++i) {
        if (embeddedCliCheckTokenEndswith(tokenizedStr, token, i))
            return i;
    }
    return 0;
}

uint16_t embeddedCliGetTokenCount(const char* tokenizedStr) {
    if (tokenizedStr == NULL || tokenizedStr[0] == '\0')
        return 0;

    int i = 0;
    uint16_t tokenCount = 1;
    while (true) {
        if (tokenizedStr[i] == '\0') {
            if (tokenizedStr[i + 1] == '\0')
                break;
            ++tokenCount;
        }
        ++i;
    }

    return tokenCount;
}

static void navigateHistory(EmbeddedCli* cli, bool navigateUp) {
    PREPARE_IMPL(cli);
    if (impl->history.itemsCount == 0 ||
        (navigateUp && impl->history.current == impl->history.itemsCount) ||
        (!navigateUp && impl->history.current == 0))
        return;

    clearCurrentLine(cli);

    printInvitation(cli);

    if (navigateUp)
        ++impl->history.current;
    else
        --impl->history.current;

    const char* item = historyGet(&impl->history, impl->history.current);
    // simple way to handle empty command the same way as others
    if (item == NULL)
        item = "";
    uint16_t len = (uint16_t)strlen(item);
    memcpy(impl->cmdBuffer, item, len);
    impl->cmdBuffer[len] = '\0';
    impl->cmdSize = len;

    writeToOutput(cli, impl->cmdBuffer);
    impl->inputLineLength = impl->cmdSize;
    impl->cursorPos = 0;

    printLiveAutocompletion(cli);
}

static void onEscapedInput(EmbeddedCli* cli, char c) {
    PREPARE_IMPL(cli);

    if (c >= 64 && c <= 126) {
        // handle escape sequence
        UNSET_U8FLAG(impl->flags, CLI_FLAG_ESCAPE_MODE);

        if (c == 'A' || c == 'B') {
            // treat \e[..A as cursor up and \e[..B as cursor down
            // there might be extra chars between [ and A/B, just ignore them
            navigateHistory(cli, c == 'A');
        } else if (c == 'C' && impl->cursorPos > 0) {
            impl->cursorPos--;
            writeToOutput(cli, escSeqCursorRight);
        } else if (c == 'D' && impl->cursorPos < strlen(impl->cmdBuffer)) {
            impl->cursorPos++;
            writeToOutput(cli, escSeqCursorLeft);
        }
    }
}

static void onCharInput(EmbeddedCli* cli, char c) {
    PREPARE_IMPL(cli);

    // have to reserve two extra chars for command ending (used in tokenization)
    if (impl->cmdSize + 2 >= impl->cmdMaxSize)
        return;

    size_t insertPos = strlen(impl->cmdBuffer) - impl->cursorPos;

    memmove(&impl->cmdBuffer[insertPos + 1], &impl->cmdBuffer[insertPos],
            impl->cursorPos + 1);
    ++impl->cmdSize;
    ++impl->inputLineLength;
    impl->cmdBuffer[insertPos] = c;
    if (impl->cursorPos > 0)
        writeToOutput(cli, escSeqInsertChar);  // Insert Character

    cli->writeChar(cli, c);
}

static void onControlInput(EmbeddedCli* cli, char c) {
    PREPARE_IMPL(cli);

    // process \r\n and \n\r as single \r\n command
    if ((impl->lastChar == '\r' && c == '\n') ||
        (impl->lastChar == '\n' && c == '\r'))
        return;

    if (c == '\r' || c == '\n') {
        // try to autocomplete command and then process it
        onAutocompleteRequest(cli, false);

        writeToOutput(cli, lineBreak);

        if (impl->cmdSize > 0)
            parseCommand(cli);
        impl->cmdSize = 0;
        impl->cmdBuffer[impl->cmdSize] = '\0';
        impl->inputLineLength = 0;
        impl->history.current = 0;
        impl->cursorPos = 0;

        printInvitation(cli);
    } else if ((c == '\b' || c == 0x7F) &&
               ((impl->cmdSize - impl->cursorPos) > 0)) {
        // remove char from screen
        writeToOutput(cli, escSeqCursorLeft);  // Move cursor to left
        writeToOutput(cli, escSeqDeleteChar);  // And remove character
        // and from buffer
        size_t insertPos = strlen(impl->cmdBuffer) - impl->cursorPos;
        memmove(&impl->cmdBuffer[insertPos - 1], &impl->cmdBuffer[insertPos],
                impl->cursorPos + 1);
        --impl->cmdSize;
    } else if (c == '\t') {
        onAutocompleteRequest(cli, true);
    } else if (c == 0x04 &&
               IS_FLAG_SET(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED)) {
        // Ctrl+D in sub interpreter mode exits sub interpreter
        writeToOutput(cli, lineBreak);  // print new line
        embeddedCliExitSubInterpreter(cli);
        UNSET_U8FLAG(impl->flags, CLI_FLAG_INIT_COMPLETE);  // print invitation
    }
}

static void parseCommand(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);

    bool isEmpty = true;

    for (int i = 0; i < impl->cmdSize; ++i) {
        if (impl->cmdBuffer[i] != ' ') {
            isEmpty = false;
            break;
        }
    }
    // do not process empty commands
    if (isEmpty)
        return;
    // push command to history before buffer is modified
    historyPut(&impl->history, impl->cmdBuffer);

    char* cmdName = NULL;
    char* cmdArgs = NULL;
    bool nameFinished = false;

    // find command name and command args inside command buffer
    for (int i = 0; i < impl->cmdSize; ++i) {
        char c = impl->cmdBuffer[i];

        if (c == ' ') {
            // all spaces between name and args are filled with zeros
            // so name is a correct null-terminated string
            if (cmdArgs == NULL)
                impl->cmdBuffer[i] = '\0';
            if (cmdName != NULL)
                nameFinished = true;

        } else if (cmdName == NULL) {
            cmdName = &impl->cmdBuffer[i];
        } else if (cmdArgs == NULL && nameFinished) {
            cmdArgs = &impl->cmdBuffer[i];
        }
    }

    // we keep two last bytes in cmd buffer reserved so cmdSize is always by 2
    // less than cmdMaxSize
    impl->cmdBuffer[impl->cmdSize + 1] = '\0';
    if (cmdArgs == NULL) {
        cmdArgs = &impl->cmdBuffer[impl->cmdSize + 1];
    }

    if (cmdName == NULL)
        return;

    // if sub interpreter is set, use it to process command
    if (IS_FLAG_SET(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED)) {
        CliCommand command;
        command.name = cmdName;
        command.args = cmdArgs;

        impl->subInterpreterOnCmd(cli, &command);
        return;
    }

    // try to find command in bindings
    for (int i = 0; i < impl->bindingsCount; ++i) {
        if (strcmp(cmdName, impl->bindings[i].name) == 0) {
            if (impl->bindings[i].func == NULL)
                break;

            // currently, output is blank line, so we can just print directly
            SET_FLAG(impl->flags, CLI_FLAG_DIRECT_PRINT);
            // check if help was requested (help is printed when no other options are
            // set)
            if (cmdArgs != NULL && (strcmp(cmdArgs, "-h") == 0 ||
                                    strcmp(cmdArgs, "--help") == 0)) {
                printBindingUsage(cli, &impl->bindings[i]);
            } else {
                if (impl->bindings[i].autoTokenizeArgs)
                    embeddedCliTokenizeArgs(cmdArgs);

                impl->currentBinding = i;
                impl->bindings[i].func(cli, cmdArgs, impl->bindings[i].context);
                impl->currentBinding = impl->bindingsCount + 1;
            }
            UNSET_U8FLAG(impl->flags, CLI_FLAG_DIRECT_PRINT);
            return;
        }
    }

    // command not found in bindings or binding was null
    // try to call default callback
    if (cli->onCommand != NULL) {
        CliCommand command;
        command.name = cmdName;
        command.args = cmdArgs;

        // currently, output is blank line, so we can just print directly
        SET_FLAG(impl->flags, CLI_FLAG_DIRECT_PRINT);
        cli->onCommand(cli, &command);
        UNSET_U8FLAG(impl->flags, CLI_FLAG_DIRECT_PRINT);
    } else {
        onUnknownCommand(cli, cmdName);
    }
}

#include <ctype.h>

static const char* toUpper(EmbeddedCli* cli, const char* str) {
    PREPARE_IMPL(cli);
    char* buf = impl->cmdBuffer;
    int i = 0;
    while (str[i] && i < impl->cmdMaxSize - 1) {
        buf[i] = (char)toupper(str[i]);
        ++i;
    }
    buf[i] = 0;
    return buf;
}

static void printBindingHelp(EmbeddedCli* cli, CliCommandBinding* binding) {
    writeToOutputColor(cli, " * ", CLI_HELP_HEADER_COLOR);
    writeToOutputColor(cli, binding->name, CLI_HELP_HEADER_COLOR);
    writeToOutput(cli, lineBreak);
    cli->writeChar(cli, '\t');
    if (binding->help != NULL) {
        writeToOutputColor(cli, binding->help, CLI_HELP_CONTENT_COLOR);
    } else {
        writeToOutputColor(cli, "No help provided", CLI_HELP_CONTENT_COLOR);
    }
    writeToOutput(cli, lineBreak);
}

static void printBindingUsage(EmbeddedCli* cli, CliCommandBinding* binding) {
    writeToOutputColor(cli, toUpper(cli, binding->name), CLI_HELP_HEADER_COLOR);
    writeToOutput(cli, " - ");
    if (binding->help != NULL) {
        writeToOutputColor(cli, binding->help, CLI_HELP_CONTENT_COLOR);
    } else {
        writeToOutputColor(cli, "No help provided", CLI_HELP_CONTENT_COLOR);
    }
    writeToOutput(cli, lineBreak);
    if (binding->usage != NULL) {
        writeToOutputColor(cli, "Usage: ", CLI_HELP_HEADER_COLOR);
        writeToOutputColor(cli, binding->usage, CLI_HELP_CONTENT_COLOR);
    } else {
        writeToOutputColor(cli, "No usage provided", CLI_HELP_HEADER_COLOR);
    }
    writeToOutput(cli, lineBreak);
}

static void initInternalBindings(EmbeddedCli* cli) {
    static CliCommandBinding b = {
        onHelp, "help", "help [cmd]", "Print list of commands", true, NULL,
    };
    embeddedCliAddBinding(cli, b);
    static CliCommandBinding c = {
        onClear, "clear", "clear", "Clear terminal screen", false, NULL,
    };
    embeddedCliAddBinding(cli, c);
#ifdef __CORTEX_M
    static CliCommandBinding d = {
        onReboot, "reboot", "reboot", "Reboot device", false, NULL,
    };
    embeddedCliAddBinding(cli, d);
#endif
}

static void onHelp(EmbeddedCli* cli, char* tokens, void* context) {
    UNUSED(context);
    PREPARE_IMPL(cli);

    if (impl->bindingsCount == 0) {
        writeToOutputColor(cli, "No commands available", CLI_ERROR_COLOR);
        writeToOutput(cli, lineBreak);
        return;
    }

    uint16_t tokenCount = embeddedCliGetTokenCount(tokens);
    if (tokenCount == 0) {
        for (int i = 0; i < impl->bindingsCount; ++i) {
            printBindingHelp(cli, &impl->bindings[i]);
        }
        writeToOutputColor(
            cli, " Tips: use \"help [cmd]\" or \"[cmd] -h\" for more info",
            CLI_HELP_HEADER_COLOR);
        writeToOutput(cli, lineBreak);
    } else if (tokenCount == 1) {
        // try find command
        CliCommandBinding* binding = NULL;
        const char* cmdName = embeddedCliGetToken(tokens, 1);
        for (int i = 0; i < impl->bindingsCount; ++i) {
            if (strcmp(impl->bindings[i].name, cmdName) == 0) {
                binding = &impl->bindings[i];
                break;
            }
        }
        if (binding != NULL) {
            printBindingUsage(cli, binding);
        } else {
            onUnknownCommand(cli, cmdName);
        }
    } else {
        writeToOutputColor(cli, "help: Too many arguments", CLI_ERROR_COLOR);
        writeToOutput(cli, lineBreak);
    }
}

static void onClear(EmbeddedCli* cli, char* tokens, void* context) {
    UNUSED(tokens);
    UNUSED(context);
    clearCurrentLine(cli);
    writeToOutput(cli, "\033[2J\033[1;1H");
}

#ifdef __CORTEX_M
static void onReboot(EmbeddedCli* cli, char* tokens, void* context) {
    UNUSED(tokens);
    UNUSED(context);
    writeToOutputColor(cli, "Rebooting...", CLI_WARNING_COLOR);
    writeToOutput(cli, lineBreak);
    m_delay_ms(100);
    // reboot
    NVIC_SystemReset();
}
#endif

static void onUnknownCommand(EmbeddedCli* cli, const char* name) {
    writeToOutputColor(cli, name, CLI_ERROR_COLOR);
    writeToOutputColor(cli, ": Command not found", CLI_ERROR_COLOR);
    writeToOutput(cli, lineBreak);
    writeToOutputColor(cli, "Check \"help\" for available commands",
                       CLI_WARNING_COLOR);
    writeToOutput(cli, lineBreak);
}

static AutocompletedCommand getAutocompletedCommand(EmbeddedCli* cli,
                                                    const char* prefix) {
    AutocompletedCommand cmd = {NULL, 0, 0};

    size_t prefixLen = strlen(prefix);

    PREPARE_IMPL(cli);
    if (impl->bindingsCount == 0 || prefixLen == 0)
        return cmd;

    for (int i = 0; i < impl->bindingsCount; ++i) {
        const char* name = impl->bindings[i].name;
        size_t len = strlen(name);

        // unset autocomplete flag
        UNSET_U8FLAG(impl->bindingsFlags[i], BINDING_FLAG_AUTOCOMPLETE);

        if (len < prefixLen)
            continue;

        // check if this command is candidate for autocomplete
        bool isCandidate = true;
        for (size_t j = 0; j < prefixLen; ++j) {
            if (prefix[j] != name[j]) {
                isCandidate = false;
                break;
            }
        }
        if (!isCandidate)
            continue;

        impl->bindingsFlags[i] |= BINDING_FLAG_AUTOCOMPLETE;

        if (cmd.candidateCount == 0 || len < cmd.autocompletedLen)
            cmd.autocompletedLen = (uint16_t)len;

        ++cmd.candidateCount;

        if (cmd.candidateCount == 1) {
            cmd.firstCandidate = name;
            continue;
        }

        for (size_t j = impl->cmdSize; j < cmd.autocompletedLen; ++j) {
            if (cmd.firstCandidate[j] != name[j]) {
                cmd.autocompletedLen = (uint16_t)j;
                break;
            }
        }
    }

    return cmd;
}

static void printLiveAutocompletion(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);

    if (IS_FLAG_SET(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED))
        return;
    if (!IS_FLAG_SET(impl->flags, CLI_FLAG_AUTOCOMPLETE_ENABLED))
        return;

    AutocompletedCommand cmd = getAutocompletedCommand(cli, impl->cmdBuffer);

    if (cmd.candidateCount == 0) {
        cmd.autocompletedLen = impl->cmdSize;
    }

    // save cursor location
    writeToOutput(cli, escSeqCursorSave);

    moveCursor(cli, impl->cursorPos, CURSOR_DIRECTION_FORWARD);

    if (IS_FLAG_SET(impl->flags, CLI_FLAG_COLOR_OUTPUT_ENABLED)) {
        writeToOutput(cli, CLI_AUTOCOMPLETE_COLOR);
    }
    // print live autocompletion (or nothing, if it doesn't exist)
    for (size_t i = impl->cmdSize; i < cmd.autocompletedLen; ++i) {
        cli->writeChar(cli, cmd.firstCandidate[i]);
    }
    if (IS_FLAG_SET(impl->flags, CLI_FLAG_COLOR_OUTPUT_ENABLED)) {
        writeToOutput(cli, FMT_RESET);
    }
    // replace with spaces previous autocompletion
    for (size_t i = cmd.autocompletedLen; i < impl->inputLineLength; ++i) {
        cli->writeChar(cli, ' ');
    }
    impl->inputLineLength = cmd.autocompletedLen;

    // restore cursor
    writeToOutput(cli, escSeqCursorRestore);
}

static void onAutocompleteRequest(EmbeddedCli* cli, bool printCandidates) {
    PREPARE_IMPL(cli);
    if (IS_FLAG_SET(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED))
        return;
    AutocompletedCommand cmd = getAutocompletedCommand(cli, impl->cmdBuffer);

    if (cmd.candidateCount == 0)
        return;

    if (cmd.candidateCount == 1 || cmd.autocompletedLen > impl->cmdSize) {
        // can copy from index cmdSize, but prefix is the same, so copy everything
        memcpy(impl->cmdBuffer, cmd.firstCandidate, cmd.autocompletedLen);
        if (cmd.candidateCount == 1) {
            impl->cmdBuffer[cmd.autocompletedLen] = ' ';
            ++cmd.autocompletedLen;
        }
        impl->cmdBuffer[cmd.autocompletedLen] = '\0';

        writeToOutput(cli, &impl->cmdBuffer[impl->cmdSize - impl->cursorPos]);
        impl->cmdSize = cmd.autocompletedLen;
        impl->inputLineLength = impl->cmdSize;
        impl->cursorPos = 0;  // Cursor has been moved to the end
        return;
    }
    if (!printCandidates)
        return;
    // with multiple candidates when we already completed to common prefix
    // we show all candidates and print input again
    // we need to completely clear current line since it begins with invitation
    // clearCurrentLine(cli);
    writeToOutput(cli, lineBreak);

    for (int i = 0; i < impl->bindingsCount; ++i) {
        // autocomplete flag is set for all candidates by last call to
        // getAutocompletedCommand
        if (!(impl->bindingsFlags[i] & BINDING_FLAG_AUTOCOMPLETE))
            continue;

        const char* name = impl->bindings[i].name;

        writeToOutput(cli, name);
        writeToOutput(cli, lineBreak);
    }

    printInvitation(cli);
    writeToOutput(cli, impl->cmdBuffer);

    impl->inputLineLength = impl->cmdSize;
}

static void clearCurrentLine(EmbeddedCli* cli) {
    PREPARE_IMPL(cli);
    size_t len = IS_FLAG_SET(impl->flags, CLI_FLAG_SUB_INTERPRETER_ENABLED)
                     ? strlen(impl->subInterpreterInvitation)
                     : strlen(impl->invitation);
    len += impl->inputLineLength;

    // cli->writeChar(cli, '\r');
    // for (size_t i = 0; i < len; ++i) {
    //   cli->writeChar(cli, ' ');
    // }
    // cli->writeChar(cli, '\r');
    writeToOutput(cli, "\033[2K\r");
    impl->inputLineLength = 0;
    impl->cursorPos = 0;
}

static void writeToOutput(EmbeddedCli* cli, const char* str) {
    size_t len = strlen(str);
    if (len == 0)
        return;

    for (size_t i = 0; i < len; ++i) {
        cli->writeChar(cli, str[i]);
    }
}

static void writeToOutputColor(EmbeddedCli* cli, const char* str,
                               const char* fmt_string) {
    PREPARE_IMPL(cli);
    if (IS_FLAG_SET(impl->flags, CLI_FLAG_COLOR_OUTPUT_ENABLED)) {
        writeToOutput(cli, fmt_string);
        writeToOutput(cli, str);
        writeToOutput(cli, FMT_RESET);
    } else {
        writeToOutput(cli, str);
    }
}

static void moveCursor(EmbeddedCli* cli, uint16_t count, bool direction) {
    // Check if we need to send any command
    if (count == 0)
        return;

    // 5 = uint16_t max, 3 = escape sequence, 1 = string termination
    char escBuffer[5 + 3 + 1] = {0};
    char dirChar = direction ? escSeqCursorRight[2] : escSeqCursorLeft[2];
    sprintf(escBuffer, "\x1B[%u%c", count, dirChar);
    writeToOutput(cli, escBuffer);
}

static bool isControlChar(char c) {
    return c == '\r' || c == '\n' || c == '\b' || c == '\t' || c == 0x7F ||
           c == 0x04;
}

static bool isDisplayableChar(char c) {
    return (c >= 32 && c <= 126);
}

static uint16_t fifoBufAvailable(FifoBuf* buffer) {
    if (buffer->back >= buffer->front)
        return (uint16_t)(buffer->back - buffer->front);
    else
        return (uint16_t)(buffer->size - buffer->front + buffer->back);
}

static char fifoBufPop(FifoBuf* buffer) {
    char a = '\0';
    if (buffer->front != buffer->back) {
        a = buffer->buf[buffer->front];
        buffer->front = (uint16_t)(buffer->front + 1) % buffer->size;
    }
    return a;
}

static bool fifoBufPush(FifoBuf* buffer, char a) {
    uint16_t newBack = (uint16_t)(buffer->back + 1) % buffer->size;
    if (newBack != buffer->front) {
        buffer->buf[buffer->back] = a;
        buffer->back = newBack;
        return true;
    }
    return false;
}

static bool historyPut(CliHistory* history, const char* str) {
    size_t len = strlen(str);
    // each item is ended with \0 so, need to have that much space at least
    if (history->bufferSize < len + 1)
        return false;

    // remove str from history (if it's present) so we don't get duplicates
    historyRemove(history, str);

    size_t usedSize;
    // remove old items if new one can't fit into buffer
    while (history->itemsCount > 0) {
        const char* item = historyGet(history, history->itemsCount);
        size_t itemLen = strlen(item);
        usedSize = ((size_t)(item - history->buf)) + itemLen + 1;

        size_t freeSpace = history->bufferSize - usedSize;

        if (freeSpace >= len + 1)
            break;

        // space not enough, remove last element
        --history->itemsCount;
    }
    if (history->itemsCount > 0) {
        // when history not empty, shift elements so new item is first
        memmove(&history->buf[len + 1], history->buf, usedSize);
    }
    memcpy(history->buf, str, len + 1);
    ++history->itemsCount;

    return true;
}

static const char* historyGet(CliHistory* history, uint16_t item) {
    if (item == 0 || item > history->itemsCount)
        return NULL;

    // items are stored in the same way (separated by \0 and counted from 1),
    // so can use this call
    return embeddedCliGetToken(history->buf, item);
}

static void historyRemove(CliHistory* history, const char* str) {
    if (str == NULL || history->itemsCount == 0)
        return;
    char* item = NULL;
    uint16_t itemPosition;
    for (itemPosition = 1; itemPosition <= history->itemsCount;
         ++itemPosition) {
        // items are stored in the same way (separated by \0 and counted from 1),
        // so can use this call
        item = embeddedCliGetTokenVariable(history->buf, itemPosition);
        if (strcmp(item, str) == 0) {
            break;
        }
        item = NULL;
    }
    if (item == NULL)
        return;

    --history->itemsCount;
    if (itemPosition == (history->itemsCount + 1)) {
        // if this is a last element, nothing is remaining to move
        return;
    }

    size_t len = strlen(item);
    size_t remaining =
        (size_t)(history->bufferSize - (item + len + 1 - history->buf));
    // move everything to the right of found item
    memmove(item, &item[len + 1], remaining);
}

static uint16_t getPositivePos(const char* tokenizedStr, int16_t pos) {
    if (pos < 0)
        pos = embeddedCliGetTokenCount(tokenizedStr) + pos + 1;
    if (pos < 1)
        pos = 1;
    return (uint16_t)pos;
}

static uint16_t getTokenPosition(const char* tokenizedStr, uint16_t pos) {
    if (tokenizedStr == NULL || pos == 0)
        return CLI_TOKEN_NPOS;
    uint16_t i = 0;
    uint16_t tokenCount = 1;
    while (true) {
        if (tokenCount == pos)
            break;

        if (tokenizedStr[i] == '\0') {
            ++tokenCount;
            if (tokenizedStr[i + 1] == '\0')
                break;
        }

        ++i;
    }

    if (tokenizedStr[i] != '\0')
        return i;
    else
        return CLI_TOKEN_NPOS;
}
