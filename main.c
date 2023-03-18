#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>

#ifdef _WIN32
#include <conio.h>
#define CLEAR_SCREEN() system("cls")
#define PAUSE() system("pause")
#define GET_KEY() getch()
#define SET_COLOR(color) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color)
#else
#include <termios.h>
#include <unistd.h>
#define CLEAR_SCREEN() system("clear")
#define PAUSE() system("read -p 'Press any key to continue...' var")
#define GET_KEY() getchar()
#define SET_COLOR(color) printf("\033[0;%dm", color)
#endif

struct Note
{
    struct MenuItem *menuItem;
    char *title;
    char *content;
    struct Date
    {
        int day;
        int month;
        int year;
        int hour;
        int minute;
    } date;
};

struct NoteNode
{
    struct Note *note;
    struct NoteNode *next;
};

struct TranslationNode
{
    char *key;
    char *value;
    struct TranslationNode *next;
};

struct MenuItem
{
    char *key;
    char *title;
    bool getTranslation;
    void (*action)(void *);
};

struct Config
{
    int MAX_TITLE_LENGTH;
    int MAX_MESSAGE_LENGTH;
    int MAX_CONTENT_LENGTH;
    int MAX_DATE_LENGTH;
    int MAX_NOTES_COUNT;
    enum appLanguage
    {
        ENGLISH,
        ROMANIAN
    } language;
    enum appColor
    {
        RED = 12,
        GREEN = 10,
        BLUE = 9,
        YELLOW = 14,
        WHITE = 15,
    } color;
};

static struct NoteNode *notesList = NULL;
static struct Note *currentStateNote = NULL;
static struct Config *config = NULL;
static struct TranslationNode *translations = NULL;

void showMenu(struct MenuItem **, char *, char *);
void addNoteAction();
void addNoteToList(struct Note *);
void deleteNoteAction();
void editNoteAction();
void viewNotes();
void changeLanguageAction();
void changeColorAction();
char *generateId();
void *getInput(char *, const char *(void *), bool);
char *formatTime(int, int);
char *formatDate(int, int, int);
void handleMenuInput(struct MenuItem **, int *, void (*)(void *), char *, char *);
void deleteNote(void *);
int getMenuItemCount(struct MenuItem **);
void saveNotesInFile();
int getFileSize(FILE *);
void getSystemDate(void *);
void getDate(void *);
void parseDate(char *, struct Date *);
void changeLanguage(void *);
void changeColor(void *);
void updateTranslations();
char *getTranslation(const char *, bool);
char *addDynamicValueToString(char *, char *);
void printList();

void exitApp()
{
    exit(0);
}

void back()
{
    return;
}

void restartCurrentStateNote()
{
    free(currentStateNote->menuItem->key);
    free(currentStateNote->menuItem);
    free(currentStateNote->title);
    free(currentStateNote->content);
    free(currentStateNote);
    currentStateNote = NULL;
}

int getFileSize(FILE *file)
{
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

int getMenuItemCount(struct MenuItem **menuItems)
{
    int count = 0;
    while (menuItems[count] != NULL)
    {
        count++;
    }
    return count;
}

void getSystemDate(void *_)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    currentStateNote->date.day = tm->tm_mday;
    currentStateNote->date.month = tm->tm_mon + 1;
    currentStateNote->date.year = tm->tm_year + 1900;
    currentStateNote->date.hour = tm->tm_hour;
    currentStateNote->date.minute = tm->tm_min;
}

const char *titleValidator(void *input)
{
    char *title = (char *)input;
    if (strlen(title) < 3)
    {
        return "Title must be at least 3 characters! \n";
    }
    if (strlen(title) > 100)
    {
        return "Title must be less than 100 characters! \n";
    }
    if (strcmp(title, " ") == 0)
    {
        return "Title must not start with a space! \n";
    }
    if (input == NULL || strcmp(title, "") == 0)
    {
        return "Title must not be empty! \n";
    }
    return NULL;
}

const char *contentValidator(void *input)
{
    char *content = (char *)input;
    if (strlen(content) < 3)
    {
        return "Content must be at least 3 characters! \n";
    }
    if (strlen(content) > 1000)
    {
        return "Content must be less than 1000 characters! \n";
    }
    if (content[0] == ' ')
    {
        return "Content must not start with a space! \n";
    }
    if (input == NULL || strcmp(content, "") == 0)
    {
        return "Content must not be empty! \n";
    }
    return NULL;
}

const char *dateValidator(void *input)
{
    char *date = (char *)input;
    int day, month, year, hour, minute;

    if (strlen(date) != config->MAX_DATE_LENGTH)
    {
        return "Invalid date format: must be in format dd.mm.yyyy hh:mm!";
    }

    if (sscanf(date, "%d.%d.%d %d:%d", &day, &month, &year, &hour, &minute) != 5)
    {
        return "Invalid date format: must be in format dd.mm.yyyy hh:mm!";
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    if (hour < 0 || hour > 23)
    {
        return "Hour must be between 0 and 23! \n";
    }

    if (day == tm->tm_mday && month == tm->tm_mon + 1 && year == tm->tm_year + 1900 && hour > tm->tm_hour)
    {
        return "Hour must be less than current hour!s \n";
    }

    if (minute < 0 || minute > 59)
    {
        return "Minute must be between 0 and 59! \n";
    }

    if (day == tm->tm_mday && month == tm->tm_mon + 1 && year == tm->tm_year + 1900 && hour == tm->tm_hour && minute > tm->tm_min)
    {
        return "Minute must be less than current minute! \n";
    }

    if (day < 1 || day > 31)
    {
        return "Day must be between 1 and 31! \n";
    }

    if (day > tm->tm_mday && month == tm->tm_mon + 1 && year == tm->tm_year + 1900)
    {
        return "Day must be less than current day! \n";
    }

    if (month < 1 || month > 12)
    {
        return "Month must be between 1 and 12! \n";
    }

    if (month > tm->tm_mon + 1 && year == tm->tm_year + 1900)
    {
        return "Month must be less than current month! \n";
    }

    if (year < 1900 || year > tm->tm_year + 1900)
    {
        return "Year must be between 1900 and current year! \n";
    }

    return NULL;
}

void getDate(void *_)
{
    CLEAR_SCREEN();
    char *dateText = getInput(getTranslation("getInputDate", true), dateValidator, false);
    struct Date *date = malloc(sizeof(struct Date));
    parseDate(dateText, date);
    currentStateNote->date.day = date->day;
    currentStateNote->date.month = date->month;
    currentStateNote->date.year = date->year;
    currentStateNote->date.hour = date->hour;
    currentStateNote->date.minute = date->minute;
    free(date);
    free(dateText);
}

void parseDate(char *dateString, struct Date *date)
{
    char dayStr[3], monthStr[3], yearStr[5], hourStr[3], minuteStr[3];
    int i = 0, j = 0;
    while (dateString[i] != '.' && i < 2)
        dayStr[j++] = dateString[i++];
    dayStr[j] = '\0';
    i++;

    j = 0;
    while (dateString[i] != '.' && i < 5)
        monthStr[j++] = dateString[i++];
    monthStr[j] = '\0';
    i++;
    j = 0;

    while (dateString[i] != ' ' && i < 10)
        yearStr[j++] = dateString[i++];
    yearStr[j] = '\0';
    i++;
    j = 0;

    while (dateString[i] != ':' && i < 13)
        hourStr[j++] = dateString[i++];
    hourStr[j] = '\0';
    i++;
    j = 0;

    while (dateString[i] != ':' && i < config->MAX_DATE_LENGTH)
        minuteStr[j++] = dateString[i++];
    minuteStr[j] = '\0';

    date->day = atoi(dayStr);
    date->month = atoi(monthStr);
    date->year = atoi(yearStr);
    date->hour = atoi(hourStr);
    date->minute = atoi(minuteStr);
}

void addNoteToList(struct Note *note)
{
    struct NoteNode *NoteNode = malloc(sizeof(struct NoteNode));
    struct Note *noteCopy = malloc(sizeof(struct Note));
    noteCopy->title = malloc(sizeof(char *) * config->MAX_TITLE_LENGTH);
    noteCopy->content = malloc(sizeof(char *) * config->MAX_CONTENT_LENGTH);
    noteCopy->menuItem = malloc(sizeof(struct MenuItem));
    noteCopy->menuItem->key = malloc(sizeof(char *) * 11);
    noteCopy->menuItem->title = malloc(sizeof(char *) * config->MAX_TITLE_LENGTH);
    strcpy(noteCopy->title, note->title);
    strcpy(noteCopy->content, note->content);
    strcpy(noteCopy->menuItem->key, note->menuItem->key);
    strcpy(noteCopy->menuItem->title, note->menuItem->title);
    noteCopy->date.day = note->date.day;
    noteCopy->date.month = note->date.month;
    noteCopy->date.year = note->date.year;
    noteCopy->date.hour = note->date.hour;
    noteCopy->date.minute = note->date.minute;
    NoteNode->note = noteCopy;
    NoteNode->next = NULL;
    if (notesList == NULL)
    {
        notesList = NoteNode;
    }
    else
    {
        struct NoteNode *current = notesList;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = NoteNode;
    }
    free(note);
}

void getDateAction()
{
    struct MenuItem **menuItem = malloc(sizeof(struct MenuItem *) * 3);
    menuItem[0] = malloc(sizeof(struct MenuItem));
    menuItem[0]->key = "addDateFunction";
    menuItem[0]->title = NULL;
    menuItem[0]->action = getDate;
    menuItem[0]->getTranslation = true;
    menuItem[1] = malloc(sizeof(struct MenuItem));
    menuItem[1]->key = "getSystemDateFunction";
    menuItem[1]->title = NULL;
    menuItem[1]->action = getSystemDate;
    menuItem[1]->getTranslation = true;
    menuItem[2] = NULL;
    showMenu(menuItem, "getDateActionTitle", "getDateActionIndication");
}

void addNoteAction()
{
    CLEAR_SCREEN();
    currentStateNote = malloc(sizeof(struct Note));
    currentStateNote->title = malloc(config->MAX_TITLE_LENGTH * sizeof(char));
    currentStateNote->content = malloc(config->MAX_CONTENT_LENGTH * sizeof(char));
    strcpy(currentStateNote->title, (char *)getInput(getTranslation("getInputNoteTitle", true), titleValidator, false));
    CLEAR_SCREEN();
    strcpy(currentStateNote->content, (char *)getInput(getTranslation("getInputNoteContent", true), contentValidator, false));
    getDateAction();
    CLEAR_SCREEN();
    currentStateNote->menuItem = malloc(sizeof(struct MenuItem));
    currentStateNote->menuItem->key = malloc(sizeof(char) * 11);
    currentStateNote->menuItem->title = malloc(sizeof(char) * (config->MAX_TITLE_LENGTH + config->MAX_DATE_LENGTH + 5));
    strcpy(currentStateNote->menuItem->key, generateId());
    char *menuTitle = malloc((config->MAX_TITLE_LENGTH + config->MAX_DATE_LENGTH + 5) * sizeof(char));
    sprintf(menuTitle, "%s - %s", currentStateNote->title, formatDate(currentStateNote->date.day, currentStateNote->date.month, currentStateNote->date.year));
    strcpy(currentStateNote->menuItem->title, menuTitle);
    currentStateNote->menuItem->action = NULL;
    addNoteToList(currentStateNote);
    saveNotesInFile();
    printf("%s", getTranslation("noteAddedSuccessfully", true));
    free(menuTitle);
    PAUSE();
}

void saveNotesInFile()
{
    FILE *file = fopen("notes.txt", "w");
    if (file == NULL)
    {
        return;
    }
    struct NoteNode *current = notesList;
    while (current != NULL)
    {
        fprintf(file, "%s|%s|%s|%d|%d|%d|%d|%d\n", current->note->menuItem->key, current->note->title, current->note->content, current->note->date.day, current->note->date.month, current->note->date.year, current->note->date.hour, current->note->date.minute);
        current = current->next;
    }
    fclose(file);
}

void saveConfig()
{
    FILE *file = fopen("config.txt", "w");
    if (file == NULL)
    {
        return;
    }
    fprintf(file, "maxTitleLength|%d\n", config->MAX_TITLE_LENGTH);
    fprintf(file, "maxContentLength|%d\n", config->MAX_CONTENT_LENGTH);
    fprintf(file, "maxDateLength|%d\n", config->MAX_DATE_LENGTH);
    fprintf(file, "maxMessageLength|%d\n", config->MAX_MESSAGE_LENGTH);
    fprintf(file, "color|%d\n", (int)config->color);
    fprintf(file, "language|%d\n", (int)config->language);
    fclose(file);
}

void editNote(void *key)
{
    CLEAR_SCREEN();
    char *noteKey = (char *)key;
    struct NoteNode *currentNoteNode = notesList;
    while (currentNoteNode != NULL)
    {
        if (strcmp(currentNoteNode->note->menuItem->key, noteKey) == 0)
        {
            void *newValue = NULL;
            printf("%s", addDynamicValueToString(getTranslation("currentTitle", true), currentNoteNode->note->title));
            newValue = getInput(getTranslation("getInputNewNoteTitle", true), titleValidator, true);
            if (newValue != NULL)
            {
                currentNoteNode->note->title = realloc(currentNoteNode->note->title, sizeof(char *) * strlen(newValue));
                currentNoteNode->note->title = newValue;
            }
            newValue = NULL;
            CLEAR_SCREEN();
            printf("%s", addDynamicValueToString(getTranslation("currentContent", true), currentNoteNode->note->content));
            newValue = getInput(getTranslation("getInputNewNoteContent", true), contentValidator, true);
            if (newValue != NULL)
            {
                currentNoteNode->note->content = realloc(currentNoteNode->note->content, sizeof(char *) * strlen(newValue));
                currentNoteNode->note->content = newValue;
            }
            CLEAR_SCREEN();
            printf("%s", addDynamicValueToString(getTranslation("currentDate", true), formatDate(currentNoteNode->note->date.day, currentNoteNode->note->date.month, currentNoteNode->note->date.year)));
            newValue = getInput(getTranslation("getInputNewNoteDate", true), dateValidator, true);
            if (newValue != NULL)
            {

                char *date = malloc(sizeof(char *) * strlen(newValue));
                date = newValue;
                struct Date *newDate = malloc(sizeof(struct Date));
                parseDate(date, newDate);
                currentNoteNode->note->date.day = newDate->day;
                currentNoteNode->note->date.month = newDate->month;
                currentNoteNode->note->date.year = newDate->year;
                currentNoteNode->note->date.hour = newDate->hour;
                currentNoteNode->note->date.minute = newDate->minute;
                free(newDate);
            }
            char *menuTitle = malloc(sizeof(char *) * config->MAX_TITLE_LENGTH + config->MAX_DATE_LENGTH + 3);
            sprintf(menuTitle, "%s - %s", currentNoteNode->note->title, formatDate(currentNoteNode->note->date.day, currentNoteNode->note->date.month, currentNoteNode->note->date.year));
            currentNoteNode->note->menuItem->title = menuTitle;
            saveNotesInFile();
            CLEAR_SCREEN();
            printf("%s", getTranslation("noteEditedSuccessfully", true));
            PAUSE();
            return;
        }
        currentNoteNode = currentNoteNode->next;
    }
}

void deleteNote(void *key)
{
    char *noteKey = (char *)key;
    struct NoteNode *currentNoteNode = notesList;
    struct NoteNode *previousNoteNode = NULL;
    while (currentNoteNode != NULL)
    {
        if (strcmp(currentNoteNode->note->menuItem->key, noteKey) == 0)
        {
            if (previousNoteNode == NULL)
            {
                notesList = currentNoteNode->next;
            }
            else
            {
                previousNoteNode->next = currentNoteNode->next;
            }
            free(currentNoteNode->note->menuItem);
            free(currentNoteNode->note->title);
            free(currentNoteNode->note->content);
            free(currentNoteNode->note);
            free(currentNoteNode);
            saveNotesInFile();
            return;
        }
        previousNoteNode = currentNoteNode;
        currentNoteNode = currentNoteNode->next;
    }
}

char *generateId()
{
    char *id = malloc(11);
    for (int i = 0; i < 10; i++)
    {
        id[i] = (rand() % 10) + 48;
    }
    if (notesList == NULL)
    {
        return id;
    }
    else
    {
        const struct NoteNode *currentNoteNode = notesList;
        while (currentNoteNode != NULL)
        {

            if (strcmp(currentNoteNode->note->menuItem->key, id) == 0)
            {
                return generateId();
            }
            currentNoteNode = currentNoteNode->next;
        }
    }
    id[11] = '\0';
    return id;
}

void *getInput(char *message, const char *(validator)(void *), bool canBeEmpty)
{
    void *input = NULL;
    char buffer[256];
    printf("%s", message);
    fgets(buffer, 255, stdin);
    input = (void *)malloc(strlen(buffer) + 1);
    buffer[strlen(buffer) - 1] = '\0';
    strcpy(input, buffer);
    if (canBeEmpty && strlen(buffer) == 0)
    {
        return NULL;
    }
    while (validator(input) != NULL)
    {

        CLEAR_SCREEN();
        printf("%s", getTranslation("invalidInput", true));
        printf("%s", validator(input));
        printf("%s", message);
        fgets(buffer, 255, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        input = realloc(input, strlen(buffer) + 1);
        strcpy(input, buffer);
    }
    free(message);
    return input;
}

void viewNotes()
{
    const struct NoteNode *currentNoteNode = notesList;
    CLEAR_SCREEN();
    if (currentNoteNode == NULL)
    {
        printf("%s", getTranslation("noNotesFound", true));
        PAUSE();
        return;
    }
    while (currentNoteNode != NULL)
    {
        if (currentNoteNode == notesList)
            printf("______________________________________________________\n");
        printf("-%s", getTranslation("noteInfo", true));
        printf("      -%s", addDynamicValueToString(getTranslation("title", true), currentNoteNode->note->title));
        printf("      -%s", getTranslation("dateInfo", true));
        printf("         -%s", addDynamicValueToString(getTranslation("date", true), formatDate(currentNoteNode->note->date.day, currentNoteNode->note->date.month, currentNoteNode->note->date.year)));
        printf("         -%s", addDynamicValueToString(getTranslation("time", true), formatTime(currentNoteNode->note->date.hour, currentNoteNode->note->date.minute)));
        printf("      -%s", addDynamicValueToString(getTranslation("content", true), currentNoteNode->note->content));
        printf("______________________________________________________\n");
        currentNoteNode = currentNoteNode->next;
    }
    PAUSE();
}

char *formatDate(int day, int month, int year)
{
    char *date = malloc(11);
    if (day < 10)
    {
        date[0] = '0';
        date[1] = day + 48;
    }
    else
    {
        date[0] = (day / 10) + 48;
        date[1] = (day % 10) + 48;
    }
    date[2] = '.';
    if (month < 10)
    {
        date[3] = '0';
        date[4] = month + 48;
    }
    else
    {
        date[3] = (month / 10) + 48;
        date[4] = (month % 10) + 48;
    }
    date[5] = '.';
    date[6] = (year / 1000) + 48;
    date[7] = ((year / 100) % 10) + 48;
    date[8] = ((year / 10) % 10) + 48;
    date[9] = (year % 10) + 48;
    date[10] = '\0';
    return date;
}

char *formatTime(int hour, int minute)
{
    char *time = malloc(6);
    if (hour < 10)
    {
        time[0] = '0';
        time[1] = hour + 48;
    }
    else
    {
        time[0] = (hour / 10) + 48;
        time[1] = (hour % 10) + 48;
    }
    time[2] = ':';
    if (minute < 10)
    {
        time[3] = '0';
        time[4] = minute + 48;
    }
    else
    {
        time[3] = (minute / 10) + 48;
        time[4] = (minute % 10) + 48;
    }
    time[5] = '\0';
    return time;
}

void changeColorAction()
{
    struct MenuItem **menuItems = malloc(sizeof(struct MenuItem *) * 7);

    struct MenuItem *redMenuItem = malloc(sizeof(struct MenuItem));
    redMenuItem->title = NULL;
    redMenuItem->key = malloc(sizeof(char) * 3);
    redMenuItem->getTranslation = true;
    sprintf(redMenuItem->key, "%d", RED);
    redMenuItem->action = changeColor;
    menuItems[0] = redMenuItem;

    struct MenuItem *greenMenuItem = malloc(sizeof(struct MenuItem));
    greenMenuItem->title = NULL;
    greenMenuItem->key = malloc(sizeof(char) * 3);
    greenMenuItem->getTranslation = true;
    sprintf(greenMenuItem->key, "%d", GREEN);
    greenMenuItem->action = changeColor;
    menuItems[1] = greenMenuItem;

    struct MenuItem *blueMenuItem = malloc(sizeof(struct MenuItem));
    blueMenuItem->title = NULL;
    blueMenuItem->key = malloc(sizeof(char) * 3);
    blueMenuItem->getTranslation = true;
    sprintf(blueMenuItem->key, "%d", BLUE);
    blueMenuItem->action = changeColor;
    menuItems[2] = blueMenuItem;

    struct MenuItem *yellowMenuItem = malloc(sizeof(struct MenuItem));
    yellowMenuItem->title = NULL;
    yellowMenuItem->key = malloc(sizeof(char) * 3);
    yellowMenuItem->getTranslation = true;
    sprintf(yellowMenuItem->key, "%d", YELLOW);
    yellowMenuItem->action = changeColor;
    menuItems[3] = yellowMenuItem;

    struct MenuItem *whiteMenuItem = malloc(sizeof(struct MenuItem));
    whiteMenuItem->title = NULL;
    whiteMenuItem->key = malloc(sizeof(char) * 2);
    whiteMenuItem->getTranslation = true;
    sprintf(whiteMenuItem->key, "%d", WHITE);
    whiteMenuItem->action = changeColor;
    menuItems[4] = whiteMenuItem;

    struct MenuItem *backMenuItem = malloc(sizeof(struct MenuItem));
    backMenuItem->title = NULL;
    backMenuItem->key = "back";
    backMenuItem->getTranslation = true;
    backMenuItem->action = back;
    menuItems[5] = backMenuItem;

    menuItems[6] = NULL;

    showMenu(menuItems, "changeColorActionTitle", "changeColorActionIndication");
}

void changeLanguageAction()
{
    struct MenuItem **menuItems = malloc(sizeof(struct MenuItem *) * 4);

    struct MenuItem *englishMenuItem = malloc(sizeof(struct MenuItem));
    englishMenuItem->title = NULL;
    englishMenuItem->key = "0";
    englishMenuItem->action = changeLanguage;
    englishMenuItem->getTranslation = true;
    menuItems[0] = englishMenuItem;

    struct MenuItem *romanianMenuItem = malloc(sizeof(struct MenuItem));
    romanianMenuItem->title = NULL;
    romanianMenuItem->key = "1";
    romanianMenuItem->action = changeLanguage;
    romanianMenuItem->getTranslation = true;
    menuItems[1] = romanianMenuItem;

    struct MenuItem *backMenuItem = malloc(sizeof(struct MenuItem));
    backMenuItem->title = NULL;
    backMenuItem->key = "back";
    backMenuItem->action = back;
    backMenuItem->getTranslation = true;
    menuItems[2] = backMenuItem;

    menuItems[3] = NULL;

    showMenu(menuItems, "changeLanguageActionTitle", "changeLanguageActionIndication");
}

void changeColor(void *color)
{
    config->color = atoi(color);
    SET_COLOR(config->color);
    saveConfig();
    CLEAR_SCREEN();
    printf("%s", getTranslation("colorChangedSuccessfully", true));
    PAUSE();
}

void changeLanguage(void *language)
{
    switch (atoi(language))
    {
    case 0:
        config->language = ENGLISH;
        break;
    case 1:
        config->language = ROMANIAN;
        break;
    default:
        break;
    }
    saveConfig();
    updateTranslations();
}

void settingsAction()
{
    struct MenuItem **menuItems = malloc(sizeof(struct MenuItem *) * 4);
    struct MenuItem *changeColor = malloc(sizeof(struct MenuItem));
    changeColor->title = NULL;
    changeColor->key = "changeColorActionTitle";
    changeColor->action = changeColorAction;
    changeColor->getTranslation = true;
    menuItems[0] = changeColor;

    struct MenuItem *changeLanguage = malloc(sizeof(struct MenuItem));
    changeLanguage->title = NULL;
    changeLanguage->key = "changeLanguageActionTitle";
    changeLanguage->action = changeLanguageAction;
    changeLanguage->getTranslation = true;
    menuItems[1] = changeLanguage;

    struct MenuItem *backMenuItem = malloc(sizeof(struct MenuItem));
    backMenuItem->title = NULL;
    backMenuItem->key = "back";
    backMenuItem->action = back;
    backMenuItem->getTranslation = true;
    menuItems[2] = backMenuItem;

    menuItems[3] = NULL;

    showMenu(menuItems, "settingsActionTitle", "settingActionIndication");
}

void editNoteAction()
{
    if (notesList == NULL)
    {
        CLEAR_SCREEN();
        printf("%s", getTranslation("noNotesFound", true));
        PAUSE();
        return;
    }
    const struct NoteNode *currentNoteNode = notesList;
    struct MenuItem **menuItems = malloc(sizeof(struct MenuItem *));
    int menuItemsCount = 0;
    while (currentNoteNode != NULL)
    {
        currentNoteNode->note->menuItem->action = editNote;
        currentNoteNode->note->menuItem->getTranslation = false;
        menuItems[menuItemsCount] = currentNoteNode->note->menuItem;
        menuItemsCount++;
        menuItems = realloc(menuItems, (menuItemsCount + 1) * sizeof(struct MenuItem *));
        currentNoteNode = currentNoteNode->next;
    }
    menuItems = realloc(menuItems, (menuItemsCount + 2) * sizeof(struct MenuItem *));
    struct MenuItem *backMenuItem = malloc(sizeof(struct MenuItem));
    backMenuItem->title = NULL;
    backMenuItem->key = "back";
    backMenuItem->action = back;
    backMenuItem->getTranslation = true;
    menuItems[menuItemsCount] = backMenuItem;
    menuItems[menuItemsCount + 1] = NULL;
    showMenu(menuItems, "editNoteActionTitle", "editNoteActionIndication");
}

void deleteNoteAction()
{
    if (notesList == NULL)
    {
        CLEAR_SCREEN();
        printf("%s", getTranslation("noNotesFound", true));
        PAUSE();
        return;
    }
    const struct NoteNode *currentNoteNode = notesList;
    struct MenuItem **menuItems = malloc(sizeof(struct MenuItem *));
    int menuItemsCount = 0;
    while (currentNoteNode != NULL)
    {
        currentNoteNode->note->menuItem->action = deleteNote;
        currentNoteNode->note->menuItem->getTranslation = false;
        menuItems[menuItemsCount] = currentNoteNode->note->menuItem;
        menuItemsCount++;
        menuItems = realloc(menuItems, (menuItemsCount + 1) * sizeof(struct MenuItem *));
        currentNoteNode = currentNoteNode->next;
    }
    menuItems = realloc(menuItems, (menuItemsCount + 2) * sizeof(struct MenuItem *));
    struct MenuItem *backMenuItem = malloc(sizeof(struct MenuItem));
    backMenuItem->title = NULL;
    backMenuItem->key = "back";
    backMenuItem->action = back;
    backMenuItem->getTranslation = true;
    menuItems[menuItemsCount] = backMenuItem;
    menuItems[menuItemsCount + 1] = NULL;
    showMenu(menuItems, "deleteNoteActionTitle", "deleteNoteActionIndication");
}

bool stayInMenu(char *key)
{
    return strstr(key, "Action") != NULL;
    //modified without testing
    //TODO: test
}

void showMenu(struct MenuItem **menuItems, char *titleTranslationKey, char *instructionsTranslationKey)
{
    static int selectedItem;
    int menuItemsCount = getMenuItemCount(menuItems);
    char *title = getTranslation(titleTranslationKey, true);
    char *instructions = getTranslation(instructionsTranslationKey, false);
    if (selectedItem > menuItemsCount - 1)
    {
        selectedItem = 0;
    }
    if (selectedItem < 0)
    {
        selectedItem = menuItemsCount - 1;
    }
    CLEAR_SCREEN();
    title != NULL ? printf("---- %s ----", title) : printf("---- %s ----", getTranslation("firstMenuTitle", true));
    for (int index = 0; index < menuItemsCount; index++)
    {
        if (menuItems[index]->getTranslation)
            menuItems[index]->title = getTranslation(menuItems[index]->key, false);
        index == selectedItem ? printf(" >> %s << \n", menuItems[index]->title) : printf(" > %s < \n", menuItems[index]->title);
    }
    instructions != NULL ? printf("%s", instructions) : printf("%s", getTranslation("firstMenuIndication", false));
    handleMenuInput(menuItems, &selectedItem, menuItems[selectedItem]->action, titleTranslationKey, instructionsTranslationKey);
}

void handleMenuInput(struct MenuItem **menuItems, int *selectedItem, void (*action)(void *data), char *title, char *instructions)
{
    while (1)
    {
        if (_kbhit())
        {
            char input;
            input = GET_KEY();
            if (input == 'w' || input == 'W')
            {
                *selectedItem -= 1;
                break;
            }
            if (input == 's' || input == 'S')
            {
                *selectedItem += 1;
                break;
            }
            if (input == 'e' || input == 'E')
            {
                int itemIndex = *selectedItem;
                *selectedItem = 0;
                action(menuItems[itemIndex]->key);
                *selectedItem = itemIndex;
                stayInMenu(menuItems[itemIndex]->key) ? break : return;
            }
        }
    }
    showMenu(menuItems, title, instructions);
}

void initializeFirstItems(struct MenuItem **menuItems)
{
    struct MenuItem *addNotesMenuItem = malloc(sizeof(struct MenuItem));
    addNotesMenuItem->key = "addNoteAction";
    addNotesMenuItem->title = NULL;
    addNotesMenuItem->action = addNoteAction;
    addNotesMenuItem->getTranslation = true;
    menuItems[0] = addNotesMenuItem;

    struct MenuItem *deleteNotesMenuItem = malloc(sizeof(struct MenuItem));
    deleteNotesMenuItem->key = "deleteNoteAction";
    deleteNotesMenuItem->title = NULL;
    deleteNotesMenuItem->action = deleteNoteAction;
    deleteNotesMenuItem->getTranslation = true;
    menuItems[1] = deleteNotesMenuItem;

    struct MenuItem *editNotesMenuItem = malloc(sizeof(struct MenuItem));
    editNotesMenuItem->key = "editNoteAction";
    editNotesMenuItem->title = NULL;
    editNotesMenuItem->action = editNoteAction;
    editNotesMenuItem->getTranslation = true;
    menuItems[2] = editNotesMenuItem;

    struct MenuItem *viewNotesMenuItem = malloc(sizeof(struct MenuItem));
    viewNotesMenuItem->key = "viewNotes";
    viewNotesMenuItem->title = NULL;
    viewNotesMenuItem->action = viewNotes;
    viewNotesMenuItem->getTranslation = true;
    menuItems[3] = viewNotesMenuItem;

    struct MenuItem *settingsMenuItem = malloc(sizeof(struct MenuItem));
    settingsMenuItem->key = "settingsAction";
    settingsMenuItem->title = NULL;
    settingsMenuItem->action = settingsAction;
    settingsMenuItem->getTranslation = true;
    menuItems[4] = settingsMenuItem;

    struct MenuItem *exitMenuItem = malloc(sizeof(struct MenuItem));
    exitMenuItem->key = "exit";
    exitMenuItem->title = NULL;
    exitMenuItem->action = exitApp;
    exitMenuItem->getTranslation = true;
    menuItems[5] = exitMenuItem;

    menuItems[6] = NULL;
}

void initializeNotesList()
{
    FILE *file = fopen("notes.txt", "r");
    if (file == NULL)
    {
        return;
    }
    char *line = malloc(sizeof(char) * config->MAX_CONTENT_LENGTH);
    size_t len = 0;
    if (getFileSize(file) == 0)
    {
        return;
    }
    while (getline(&line, &len, file) != -1)
    {
        char *key = strtok(line, "|");
        char *title = strtok(NULL, "|");
        char *content = strtok(NULL, "|");
        char *day = strtok(NULL, "|");
        char *month = strtok(NULL, "|");
        char *year = strtok(NULL, "|");
        char *hour = strtok(NULL, "|");
        char *minute = strtok(NULL, "|");

        struct Note *note = malloc(sizeof(struct Note));
        note->title = title;
        note->content = content;
        note->date.day = atoi(day);
        note->date.month = atoi(month);
        note->date.year = atoi(year);
        note->date.hour = atoi(hour);
        note->date.minute = atoi(minute);
        note->menuItem = malloc(sizeof(struct MenuItem));
        note->menuItem->key = key;
        char *menuTitle = malloc(sizeof(char *) * (config->MAX_TITLE_LENGTH + config->MAX_DATE_LENGTH));
        sprintf(menuTitle, "%s - %s", note->title, formatDate(note->date.day, note->date.month, note->date.year));
        note->menuItem->title = menuTitle;
        note->menuItem->action = NULL;
        addNoteToList(note);
    }
    free(line);
    fclose(file);
}

void initConfig()
{
    FILE *file = fopen("config.txt", "r");
    config = malloc(sizeof(struct Config));
    char *line = malloc(sizeof(char) * config->MAX_CONTENT_LENGTH);
    size_t len = 0;
    if (file == NULL || getFileSize(file) == 0)
    {
        config->MAX_TITLE_LENGTH = 100;
        config->MAX_CONTENT_LENGTH = 1000;
        config->MAX_DATE_LENGTH = 20;
        config->MAX_MESSAGE_LENGTH = 100;
        config->MAX_NOTES_COUNT = 100;
        config->color = WHITE;
        config->language = ENGLISH;
        saveConfig();
        return;
    }
    while (getline(&line, &len, file) != -1)
    {
        char *key = strtok(line, "|");
        char *value = strtok(NULL, "|");
        if (strcmp(key, "maxTitleLength") == 0)
        {
            config->MAX_TITLE_LENGTH = atoi(value);
        }
        if (strcmp(key, "maxContentLength") == 0)
        {
            config->MAX_CONTENT_LENGTH = atoi(value);
        }
        if (strcmp(key, "maxDateLength") == 0)
        {
            config->MAX_DATE_LENGTH = atoi(value);
        }
        if (strcmp(key, "maxMessageLength") == 0)
        {
            config->MAX_MESSAGE_LENGTH = atoi(value);
        }
        if (strcmp(key, "maxNotesCount") == 0)
        {
            config->MAX_NOTES_COUNT = atoi(value);
        }
        if (strcmp(key, "color") == 0)
        {
            config->color = atoi(value);
        }
        if (strcmp(key, "language") == 0)
        {
            config->language = atoi(value);
        }
    }
    SET_COLOR(config->color);
    free(line);
    fclose(file);
}

char *addDynamicValueToString(char *str, char *value)
{
    char *newStr = malloc(strlen(str) + strlen(value) + 1);
    if (newStr == NULL)
    {
        return NULL;
    }
    char *p = strstr(str, "#value");
    if (p == NULL)
    {
        strcpy(newStr, str);
    }
    else
    {
        *p = '\0';
        sprintf(newStr, "%s%s%s", str, value, p + strlen("#value"));
    }
    str = realloc(str, strlen(newStr) + 1);
    strcpy(str, newStr);
    free(newStr);
    return str;
}

void updateTranslations()
{
    FILE *file = fopen("translations.txt", "r");
    char *line = malloc(sizeof(char) * config->MAX_CONTENT_LENGTH);
    size_t len = 0;
    if (file == NULL || getFileSize(file) == 0)
    {
        return;
    }
    while (getline(&line, &len, file) != -1)
    {
        char *key = strtok(line, "|");
        for (int i = 0; i < config->language; i++)
        {
            strtok(NULL, "|");
        }
        char *value = strtok(NULL, "|");
        const int valueLength = strlen(value);
        if (value[valueLength - 1] == '\n')
        {
            value[valueLength - 1] = '\0';
        }
        struct TranslationNode *current = translations;
        while (current != NULL)
        {
            if (strcmp(current->key, key) == 0)
            {
                current->value = realloc(current->value, sizeof(char) * (valueLength + 1));
                strcpy(current->value, value);
                break;
            }
            current = current->next;
        }
    }
    free(line);
    fclose(file);
}

void addTranslation(struct TranslationNode *translation)
{
    if (translations == NULL)
    {
        translations = translation;
        return;
    }
    struct TranslationNode *current = translations;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = translation;
}

char *getTranslation(const char *key, bool newLineAtEnd)
{
    struct TranslationNode *current = translations;
    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            char *value = malloc(sizeof(char) * (strlen(current->value) + 1));
            strcpy(value, current->value);
            if (newLineAtEnd && current->value[strlen(current->value) - 1] != '\n')
            {
                strcat(value, "\n");
            }
            return value;
        }
        current = current->next;
    }
    return "NOT FOUND, PLEASE ADD IT TO TRANSLATIONS FILE";
}

void initTranslations()
{
    FILE *file = fopen("translations.txt", "r");
    char *line = malloc(sizeof(char) * config->MAX_CONTENT_LENGTH);
    size_t len = 0;
    if (file == NULL || getFileSize(file) == 0)
    {
        printf("Translations file not found, WE WILL EXIT NOW");
        PAUSE();
        exitApp();
    }
    while (getline(&line, &len, file) != -1)
    {
        char *key = strtok(line, "|");
        for (int i = 0; i < config->language; i++)
        {
            strtok(NULL, "|");
        }
        char *value = strtok(NULL, "|");
        const int valueLength = strlen(value);
        if (value[valueLength - 1] == '\n')
        {
            value[valueLength - 1] = '\0';
        }
        struct TranslationNode *translation = malloc(sizeof(struct TranslationNode));
        translation->key = malloc(strlen(key) + 1);
        translation->value = malloc(strlen(value) + 1);
        strcpy(translation->key, key);
        strcpy(translation->value, value);
        translation->next = NULL;
        addTranslation(translation);
    }
    free(line);
    fclose(file);
}

int main()
{
    initConfig();
    initTranslations();
    initializeNotesList();
    struct MenuItem **firstMenuItems = malloc(sizeof(struct MenuItem *) * 7);
    initializeFirstItems(firstMenuItems);
    while (1)
    {
        showMenu(firstMenuItems, "firstMenuTitle", "firstMenuIndication");
    }
    return 0;
}
