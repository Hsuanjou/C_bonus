#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include "queue.h"

#define BUFFSIZE 64
#define TOKEN_DELIM " \t\r\n\a"
#define LINESIZE 1024

#define clear() printf("\033[H\033[J")

pid_t pid;
int jobs_n = 0;

queue *q;
int cores;
int job_id = 1;

struct Job_entry
{
    char p_name[BUFSIZ];
    int l_pid;
};

struct Job_entry *entries[50];

int list(char **args);
int cd(char **args);
int fibonacci(char **args);
int hello(char **args);

queue *queue_init(int n);
int queue_insert(queue *q, int item);
int queue_delete(queue *q);
void queue_display(queue *q);
void queue_destroy(queue *q);

char *builtin_str[] = {
    "list",
    "cd",
    "hello",
    "fibonacci"};

int (*builtin_func[])(char **) = {
    &list,
    &cd,
    &hello,
    &fibonacci};

struct Job_entry *createJob(char *p_name, int l_pid)
{

    struct Job_entry *p = malloc(sizeof(struct Job_entry));

    p->l_pid = l_pid;

    strcpy(p->p_name, p_name);

    return p;
}

char *Uabsh_read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

void startupMessage()
{
    clear();
    printf("Welcome to Uab_sh");
    char *username = getenv("USER");
    printf("\n\nUser:\t%s\n\n\n", username);

    sleep(1);
}

char **Uabsh_split_line(char *line)
{
    int bufsize = BUFFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "Uab_sh allocation error\n");
        exit(-1);
    }

    token = strtok(line, TOKEN_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += BUFFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "Uab_sh allocation error\n");
                exit(-1);
            }
        }

        token = strtok(NULL, TOKEN_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

static void sig_usr(int signo)
{
    switch (signo)
    {
    case SIGINT:
        kill(pid, SIGINT);
        break;
    case SIGTSTP:
        kill(pid, SIGTSTP);
        char command[50] = "/proc/";
        char tempid[10];
        snprintf(tempid, 10, "%d", pid);
        strcat(command, tempid);
        strcat(command, "/cmdline");

        char line[BUFSIZ];
        char *name = malloc(1024 * sizeof(char *));

        FILE *file = fopen(command, "r");

        if (file == NULL)
        {
            printf("Error reading input file input.txt\n");
            exit(-1);
        }

        while (fgets(line, BUFSIZ, file))
        {
            strcpy(name, line);
        }

        printf("%s\n", name);

        fclose(file);

        entries[jobs_n] = createJob(name, pid);
        jobs_n++;

        free(name);
        break;
    default:
        printf("received signal %d\n", signo);
    }
}

int Uabsh_launch(char **args, char *line)
{
    int status, k = 0;
    int fdout, fdin;

    pid = fork();
    if (pid == 0)
    {

        int num_args = 0;
        while (args[num_args] != NULL)
        {
            num_args++;
        }

        int j = 0, flag = 0;
        char *args_cp[100];
        for (; j < num_args; j++)
        {
            if (strcmp(args[j], "<") == 0 || strcmp(args[j], "<\n") == 0 || strcmp(args[j], ">") == 0 || strcmp(args[j], ">\n") == 0)
            {
                flag = 1;
                if (num_args > j + 1)
                {
                    if (strcmp(args[j], "<") == 0)
                    {
                        if (((fdin = open(args[j + 1], O_RDONLY, 0755)) == -1))
                        {
                            printf("Error opening file %s for output\n", args[j]);
                            exit(-1);
                        }
                        dup2(fdin, 0);
                    }
                    else if (strcmp(args[j], ">") == 0)
                    {
                        if (((fdout = open(args[j + 1], O_CREAT | O_APPEND | O_WRONLY, 0755)) == -1))
                        {
                            printf("Error opening file %s for output\n", args[j]);
                            exit(-1);
                        }
                        dup2(fdout, 1);
                    }
                }
                else
                {
                    printf("Filename not given\n");
                    return 1;
                }
            }
            else if (flag == 0)
            {
                args_cp[k] = args[j];
                k++;
            }
            else if (flag == 1)
            {
                flag = 0;
            }
        }
        args_cp[k] = NULL;
        if (execvp(args_cp[0], args_cp) == -1)
        {
            perror("Uab_sh");
        }
        exit(-1);
    }
    else if (pid > 0)
    {

        signal(SIGINT, sig_usr);
        signal(SIGTSTP, sig_usr);

        waitpid(pid, &status, WUNTRACED);

        if (WIFEXITED(status))
        {
        }
        else if (WIFSTOPPED(status))
        {
            printf("Process with pid=%d has been STOPPED\n", pid);
        }
        else if (WTERMSIG(status) == 2)
        {
            printf("Process with pid=%d has been INTERRUPTED...\n", pid);
        }
        else
        {
            printf("Child process did not terminate normally! (Code = %d)\n", status);
        }
    }
    else
    {

        perror("Uab_sh");
        exit(EXIT_FAILURE);
    }

    return 1;
}

int Uabsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int list(char **args)
{

    DIR *d;
    struct dirent *dir;
    d = opendir(getenv("PWD"));

    printf("%s\n", getenv("PWD"));
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\t", dir->d_name);
        }
        closedir(d);
        printf("\n");
    }
    return 1;
}

int cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "Uab_sh expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("Uab_sh");
        }
        else
        {
            printf("%s\n", getenv("PWD"));
        }
    }
    return 1;
}

int hello(char **args)
{
    printf("Hello world\n");
}

int fibonacci(char **args)
{
    int t1 = 0, t2 = 1, nextTerm = 0, n;

    printf("Enter the number: ");
    scanf("%d", &n);

    printf("Fibonacci: %d, %d, ", t1, t2);

    nextTerm = t1 + t2;

    while (nextTerm <= n)
    {
        printf("%d, ", nextTerm);
        t1 = t2;
        t2 = nextTerm;
        nextTerm = t1 + t2;
    }

    return 0;
}

int Uabsh_execute(char **args, char *line)
{
    int i;

    if (args[0] == NULL)
    {
        return 1;
    }

    for (i = 0; i < Uabsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }

        return Uabsh_launch(args, line);
    }
}

void Uabsh_loop(void)
{
    char *line;
    char line2[BUFSIZ];
    char *current_dir;
    char *temp_dir = malloc(1024 * sizeof(char *));
    char **args;
    int status;
    int line_num = 1;

    current_dir = getenv("PWD");
    strcat(temp_dir, current_dir);

    FILE *f = fopen(strcat(temp_dir, "/Uab_sh.log"), "wb");

    do
    {
        printf("Uab_sh> ");
        line = Uabsh_read_line();
        strcpy(line2, line);
        fprintf(f, "  %d  %s", line_num, line);
        args = Uabsh_split_line(line);
        status = Uabsh_execute(args, line2);

        free(line);
        free(args);

        line_num++;

    } while (status);

    free(temp_dir);
    fclose(f);
}

int main(int argc, char **argv)
{

    if (argv[1] == NULL)
    {
        printf("./Uab_sh <number of cores>\n");
        exit(-1);
    }
    else if (strcmp(argv[0], "hello") == 0)
    {
        Uabsh_execute(argv, 0);
    }
    else
    {
        cores = atoi(argv[1]);
    }
    q = queue_init(20);
    startupMessage();
    Uabsh_loop();

    return 0;
}