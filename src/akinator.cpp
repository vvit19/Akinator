#include "akinator.h"
#include "stack.h"
#include "utils.h"

#include <cctype>
#include <cstring>

static void   StartGame      (const char* base);
static char*  GetTree        (Node* node, char* buffer);
static void   Guess          (Node* node);
static Node*  GetObject      (Node* node, const char* name);
static void   GetSentence    (char* name);
static void   FindPath       (Node* node, stack* stk);
static void   TellAbout      (Node* node, stack* stk);
static void   DescribeObject (Node* main_node, const char* name);
static void   CompareObjects (Node* main_node, const char* name_1, const char* name_2);
static void   PrintAndTell   (const char string[]);

#define SPEAK

#ifdef SPEAK
    #define PRINT_AND_TELL(...) { \
        char spoken_text[MAX_SPEAK_LENGTH] = ""; \
        sprintf (spoken_text, __VA_ARGS__); \
        PrintAndTell (spoken_text); }
#else
    #define PRINT_AND_TELL(...) printf (__VA_ARGS__);
#endif

const int MAX_SPEAK_LENGTH  = 1024;
const int MAX_NAME_LENGTH   = 100;
const int MAX_ANSWER_LENGTH = 7;

static void PrintAndTell (const char string[])
{
    assert (string);

    printf ("%s", string);

    #ifdef SPEAK
        char spoken_text[MAX_SPEAK_LENGTH] = "";
        sprintf (spoken_text, "echo \"%s\" | festival --tts --language russian", string);
        system (spoken_text);
    #endif
}

int main (int argc, const char** argv)
{
    if (argc != 2)
    {
        PrintAndTell ("Некорректный ввод аргументов командной строки\n");
        return 1;
    }

    StartGame (argv[1]);

    return 0;
}

Node* CreateNode (Node* parent, Way mode)
{
    assert (parent);

    Node* node = (Node*) calloc (1, sizeof (Node));
    assert (node);

    if (mode == LEFT)
        parent->left  = node;
    else
        parent->right = node;

    node->parent = parent;
    node->name  = (char*) calloc (MAX_NAME_LENGTH, sizeof (char));

    return node;
}

void TreeDtor (Node* node)
{
    if (node == nullptr) return;

    free (node->name);
    node->name = nullptr;

    TreeDtor (node->left);
    free (node->left);
    node->left = nullptr;

    TreeDtor (node->right);
    free (node->right);
    node->right = nullptr;
}

static void StartGame (const char* base)
{
    Node* main_node = (Node*) calloc (1, sizeof (Node));
    main_node->name = (char*) calloc (MAX_NAME_LENGTH, sizeof (char));

    char* buffer = get_file_content (base);

    GetTree (main_node, buffer + 1);

    PrintAndTell ("Акинатор начинает разносить\n"
                  "Выбери режим: \n"
                  "1) o - отгадывание \n"
                  "2) р - расскажу о предмете из базы \n"
                  "3) с - сравню 2 предмета из базы \n");

    char ch[3] = "";
    scanf ("%s", ch);
    ClearBuffer ();

    if (strcmp (ch, "о") == 0)
    {
        PrintAndTell ("Если ответ на вопрос да - введите \"да\", если ответ нет - введите \"нет\"\n");
        Guess (main_node);
    }
    else if (strcmp (ch, "р") == 0)
    {
        PrintAndTell ("Введите название предмета: ");
        char name[MAX_NAME_LENGTH] = "";
        GetSentence (name);

        DescribeObject (main_node, name);
    }
    else if (strcmp (ch, "с") == 0)
    {
        PrintAndTell ("Введите название первого предмета: ");
        char name_1[MAX_NAME_LENGTH] = "";
        GetSentence (name_1);

        PrintAndTell ("Введите название второго предмета: ");
        char name_2[MAX_NAME_LENGTH] = "";
        GetSentence (name_2);

        if (strcmp (name_1, name_2) == 0) PrintAndTell ("Они одинаковые\n");
        else CompareObjects (main_node, name_1, name_2);
    }
    else
    {
        PrintAndTell ("Неверный ввод режима\n");
    }

    TreeDump (main_node);

    FILE* file = fopen (base, "w");
    PrintTree (main_node, file);
    fclose (file);

    TreeDtor (main_node);
    free (buffer);
}

static char* GetTree (Node* node, char* buffer)
{
    assert (node);
    assert (buffer);

    while (isspace (*buffer))
    {
        buffer++;
    }

    if (*buffer == '(') buffer++;

    int i = 0;
    for ( ; *buffer != ')' && *buffer != '\0'; buffer++)
    {
        if (*buffer == '(')
        {
            buffer = GetTree (CreateNode (node, RIGHT), buffer + 1);
            buffer = GetTree (CreateNode (node, LEFT ), buffer + 1);
        }
        else
        {
            if (*buffer == ' ')
            {
                if (! (i > 0 && !isspace (node->name[i - 1]))) continue;
            }

            if (isspace (*buffer) && *buffer != ' ') continue;

            node->name[i++] = *buffer;
        }
    }

    if (node->name[i - 1] == ' ') { node->name[i - 1] = '\0'; }

    return buffer;
}

static void Guess (Node* node)
{
    assert (node);

    char answer[MAX_ANSWER_LENGTH] = "";

    if (node->left == nullptr && node->right == nullptr)
    {
        PRINT_AND_TELL("Я знаю ответ! Это %s?\n", node->name);

        scanf ("%s", answer);
        if (strcmp (answer, "да") == 0) PrintAndTell ("Ха я гений\n");
        else
        {
            CreateNode (node, RIGHT);
            CreateNode (node, LEFT);

            PrintAndTell ("И кто же это?\n"
                          "Это ");
            ClearBuffer ();
            GetSentence (node->left->name);

            strcpy (node->right->name, node->name);

            PRINT_AND_TELL("А чем %s отличается от %s?\n"
                           "Он(а/o) ", node->left->name, node->right->name);
            GetSentence (node->name);
        }

        return;
    }

    PRINT_AND_TELL("%s?\n", node->name);

    scanf ("%s", answer);
    while (true)
    {
        if (strcmp (answer, "да") == 0) { Guess (node->left); break; }
        else if (strcmp (answer, "нет") == 0) { Guess (node->right); break; }
        else
        {
            PrintAndTell ("Некорректный ввод! Попробуйте еще\n");
            scanf ("%s", answer);
        }
    }
}

static void CompareObjects (Node* main_node, const char* name_1, const char* name_2)
{
    assert (main_node);
    assert (name_1);
    assert (name_2);

    Node* object_1 = GetObject (main_node, name_1);
    if (!object_1)
    {
        PrintAndTell ("Первого объекта в базе нет!\n");
        return;
    }

    Node* object_2 = GetObject (main_node, name_2);
    if (!object_2)
    {
        PrintAndTell ("Второго объекта в базе нет!\n");
        return;
    }

    stack stk_1 = {};
    stack_ctor (&stk_1);
    FindPath (object_1, &stk_1);

    stack stk_2 = {};
    stack_ctor (&stk_2);
    FindPath (object_2, &stk_2);

    Node* node = main_node;
    Way way_1 = LEFT;
    Way way_2 = LEFT;

    while (stk_1.size > 0 && stk_2.size > 0)
    {
        stack_pop (&stk_1, (elem_t*) &way_1);
        stack_pop (&stk_2, (elem_t*) &way_2);

        if (way_1 != way_2)
        {
            if (way_1 == LEFT)
            {
                PRINT_AND_TELL("Про %s можно сказать %s, в то время как про %s так сказать нельзя\n",
                                name_1, node->name, name_2);
            }
            else
            {
                PRINT_AND_TELL("Про %s можно сказать %s, в то время как про %s так сказать нельзя\n",
                                name_2, node->name, name_1);
            }

            stack_dtor (&stk_1);
            stack_dtor (&stk_2);

            return;
        }

        if (way_1 == LEFT) node = node->left;
        else node = node->right;
    }

    stack_dtor (&stk_1);
    stack_dtor (&stk_2);
}

static Node* GetObject (Node* node, const char* name)
{
    assert (name);

    if (!node) return nullptr;

    if (strcmp (node->name, name) == 0) return node;

    Node* object = GetObject (node->right, name);
    if (object) return object;

    object = GetObject (node->left, name);
    if (object) return object;

    return nullptr;
}

static void DescribeObject (Node* main_node, const char* name)
{
    assert (name);
    assert (main_node);

    Node* object = GetObject (main_node, name);
    if (!object)
    {
        PrintAndTell ("Такого объекта в базе нет!\n");
        return;
    }

    stack stk = {};
    stack_ctor (&stk);

    FindPath (object, &stk);
    TellAbout (main_node, &stk);
    putchar ('\n');

    stack_dtor (&stk);
}

static void FindPath (Node* node, stack* stk)
{
    assert (stk);

    if (node->parent == nullptr) { return; }

    Node* parent = node->parent;

    if (node == parent->left)
    {
        stack_push (stk, LEFT);
        FindPath (parent, stk);
    }
    else
    {
        stack_push (stk, RIGHT);
        FindPath (parent, stk);
    }
}

static void TellAbout (Node* node, stack* stk)
{
    Way way = LEFT;
    stack_pop (stk, (int*) &way);

    if (way == LEFT)
    {
        PRINT_AND_TELL("%s", node->name);
        node = node->left;
    }
    else
    {
        PRINT_AND_TELL("не %s", node->name);
        node = node->right;
    }

    if (node->left && node->right)
    {
        printf (", \n");
        TellAbout (node, stk);
    }
}

static void GetSentence (char* name)
{
    assert (name);

    int ch = getchar (), i = 0;

    while (ch != '\n')
    {
        name[i++] = ch;
        ch = getchar ();
    }

    name[i] = '\0';
}
