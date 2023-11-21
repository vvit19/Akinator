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
static void   FindWay        (Node* node, stack* stk);
static void   TellAbout      (Node* node, stack* stk);
static void   DescribeObject (Node* main_node, const char* name);
static void   CompareObjects (Node* main_node, const char* name_1, const char* name_2);

const int MAX_NAME_LENGTH = 100;
const int ANSWER_LENGTH   = 7;

int main (int /*argc*/, const char** argv)
{
    StartGame (argv[1]);
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

    printf ("Акинатор начинает разносить\n"
            "Выбери режим: \n"
            "1) o - отгадывание \n"
            "2) р - расскажу о предмете из базы \n"
            "3) с - сравню 2 предмета из базы \n");

    char ch[3] = "";
    scanf ("%s", ch);
    ClearBuffer ();

    if (strcmp (ch, "о") == 0)
    {
        printf ("Если ответ на вопрос да - введите \"да\", если ответ нет - введите \"нет\"\n");
        Guess (main_node);
    }
    else if (strcmp (ch, "р") == 0)
    {
        printf ("Введите название предмета: ");
        char name[MAX_NAME_LENGTH] = "";
        GetSentence (name);

        DescribeObject (main_node, name);
    }
    else if (strcmp (ch, "с") == 0)
    {
        printf ("Введите название первого предмета: ");
        char name_1[MAX_NAME_LENGTH] = "";
        GetSentence (name_1);

        printf ("Введите название второго предмета: ");
        char name_2[MAX_NAME_LENGTH] = "";
        GetSentence (name_2);

        if (strcmp (name_1, name_2) == 0) printf ("Они одинаковые\n");
        else CompareObjects (main_node, name_1, name_2);
    }
    else
    {
        printf ("Неверный ввод режима\n");
    }

    TreeDump  (main_node);

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

    char answer[ANSWER_LENGTH] = "";

    if (node->left == nullptr && node->right == nullptr)
    {
        printf ("Я знаю ответ! Это %s?\n", node->name);
        scanf ("%s", answer);
        if (strcmp (answer, "да") == 0) printf ("Ха я гений\n");
        else
        {
            CreateNode (node, RIGHT);
            CreateNode (node, LEFT);

            printf ("И кто же это?\n"
                    "Это ");
            ClearBuffer ();
            GetSentence (node->left->name);

            strcpy (node->right->name, node->name);

            printf ("А чем %s отличается от %s?\n"
                    "Он (а/o) ", node->left->name, node->right->name);
            GetSentence (node->name);
        }

        return;
    }

    printf ("%s?\n", node->name);

    scanf ("%s", answer);
    while (true)
    {
        if (strcmp (answer, "да") == 0) { Guess (node->left); break; }
        else if (strcmp (answer, "нет") == 0) { Guess (node->right); break; }
        else
        {
            printf ("Некорректный ввод! Попробуйте еще\n");
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
    Node* object_2 = GetObject (main_node, name_2);

    stack stk_1 = {};
    stack_ctor (&stk_1);
    FindWay (object_1, &stk_1);

    stack stk_2 = {};
    stack_ctor (&stk_2);
    FindWay (object_2, &stk_2);

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
                printf ("Про %s можно сказать %s, в то время как про %s так сказать нельзя\n",
                        name_1, node->name, name_2);
            }
            else
            {
                printf ("Про %s можно сказать %s, в то время как про %s так сказать нельзя\n",
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

    stack stk = {};
    stack_ctor (&stk);

    FindWay (object, &stk);
    TellAbout (main_node, &stk);
    putchar ('\n');

    stack_dtor (&stk);
}

static void FindWay (Node* node, stack* stk)
{
    assert (stk);

    if (node->parent == nullptr) return;

    Node* parent = node->parent;

    if (node == parent->left)
    {
        stack_push (stk, LEFT);
        FindWay (parent, stk);
    }
    else
    {
        stack_push (stk, RIGHT);
        FindWay (parent, stk);
    }
}

static void TellAbout (Node* node, stack* stk)
{
    Way way = LEFT;
    stack_pop (stk, (int*) &way);

    if (way == LEFT)
    {
        printf ("%s", node->name);
        node = node->left;
    }
    else
    {
        printf ("не %s", node->name);
        node = node->right;
    }

    if (node->left && node->right)
    {
        printf (", ");
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
