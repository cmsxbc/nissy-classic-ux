#include "shell.h"
#ifdef ENABLE_READLINE
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifndef VERSION
#define VERSION "0.0.0.dev"
#endif

static void             cleanwhitespaces(char *line);
static int              parseline(char *line, char **v);

bool
checkfiles(void)
{
	/* TODO: add more checks (other files, use checksum...) */
	FILE *f;
	char fname[strlen(tabledir)+100];
	int i;

	for (i = 0; all_pd[i] != NULL; i++) {
		strcpy(fname, tabledir);
		strcat(fname, "/");
		strcat(fname, all_pd[i]->filename);
		if ((f = fopen(fname, "rb")) == NULL)
			return false;
		else
			fclose(f);
	}

	return true;
}

static void
cleanwhitespaces(char *line)
{
	char *i;

	for (i = line; *i != 0; i++)
		if (*i == '\t' || *i == '\n')
			*i = ' ';
}

/* This function assumes that **v is large enough */
static int
parseline(char *line, char **v)
{
	char *t;
	int n = 0;
	
	cleanwhitespaces(line);

	for (t = strtok(line, " "); t != NULL; t = strtok(NULL, " "))
		strcpy(v[n++], t);

	return n;
}

void
exec_args(int c, char **v)
{
	int i;
	char line[MAXLINELEN];
	Command *cmd = NULL;
	CommandArgs *args;
	Alg *scramble;

	for (i = 0; commands[i] != NULL; i++)
		if (!strcmp(v[0], commands[i]->name))
			cmd = commands[i];

	if (cmd == NULL) {
		fprintf(stderr, "%s: command not found\n", v[0]);
		return;
	}

	args = cmd->parse_args(c-1, &v[1]);
	if (!args->success) {
		fprintf(stderr, "usage: %s\n", cmd->usage);
		goto exec_args_end;
	}

	if (args->scrstdin) {
		while (true) {
			if (fgets(line, MAXLINELEN, stdin) == NULL) {
				clearerr(stdin);
				break;
			}
			
			scramble = new_alg(line);

			printf(">>> Line: %s", line);

			if (scramble != NULL && scramble->len > 0) {
				args->scramble = scramble;
				cmd->exec(args);
				free_alg(scramble);
				args->scramble = NULL;
			} 
			fflush(stdout);
		}
	} else {
		cmd->exec(args);
	}

exec_args_end:
	free_args(args);
}

char *
command_generator(const char *text, int state)
{
    static int list_index = 0, len = 0;
    if (!state) {
        list_index = 0;
        len = strlen(text);
    }
    Command* command = NULL;
    while ((command = commands[list_index++])) {
        if (strncmp(command->name, text, len) == 0) {
            char *r = malloc(strlen(command->name) + 1);
            strcpy(r, command->name);
            return r;
        }
    };
    return NULL;
}

char *
step_generator(const char *text, int state)
{
    static int list_index = 0, len = 0;
    if (!state) {
        list_index = 0;
        len = strlen(text);
    }
    Step* step = NULL;
    while ((step = steps[list_index++])) {
        if (strncmp(step->shortname, text, len) == 0) {
            char *r = malloc(strlen(step->shortname) + 1);
            strcpy(r, step->shortname);
            return r;
        }
    }
    return NULL;
}

char *
solve_args_generator(const char *text, int state)
{
    static int list_index = 0, len = 0;
    static char* args = "mMtnoONLivapc";
    if (!state) {
        list_index = 0;
        len = strlen(text);
    }
    if (len > 3) {
        return NULL;
    }
    if (len > 0 && text[0] != '-') {
        return NULL;
    }
    while ((size_t)list_index < strlen(args)) {
        char *r = malloc(3);
        r[0] = '-';
        r[1] = args[list_index++];
        r[2] = '\0';
        return r;
    }
    return NULL;
}

#ifdef ENABLE_READLINE
char **
nissy_completion(const char *text, int start, int end)
{
    if (start == 0)
        return rl_completion_matches(text, command_generator);
    const char * typed = rl_line_buffer;
    char ** matches = NULL;
    if (!typed) {
        return NULL;
    }
    if (strncmp(typed, "solve", 5) == 0) {
        if (start <= 6) {
            matches = rl_completion_matches(text, step_generator);
        } else {
            matches = rl_completion_matches(text, solve_args_generator);
        }
    }
    return matches;
}

void
launch_readline()
{
    int i, shell_argc;
    char line[MAXLINELEN], **shell_argv;
    shell_argv = malloc(MAXNTOKENS * sizeof(char *));
    for (i = 0; i < MAXNTOKENS; i++)
        shell_argv[i] = malloc((MAXTOKENLEN+1) * sizeof(char));
    rl_readline_name = "nissy";
    rl_attempted_completion_function = nissy_completion;
    while (true) {
        char *rl_line = readline("nissy-# ");
        if (!rl_line) {
            break;
        }
        memcpy(line, rl_line, MAXLINELEN);
        shell_argc = parseline(line, shell_argv);

        if (shell_argc > 0) {
            add_history(rl_line);
            exec_args(shell_argc, shell_argv);
        }

        fflush(stdout);
        free(rl_line);
        rl_line = NULL;
    }
    for (i = 0; i < MAXNTOKENS; i++)
        free(shell_argv[i]);
    free(shell_argv);
}
#endif

void
launch(bool batchmode)
{
#ifdef ENABLE_READLINE
    if (!batchmode) {
        launch_readline();
        return;
    }
#endif


	int i, shell_argc;
	char line[MAXLINELEN], **shell_argv;

	shell_argv = malloc(MAXNTOKENS * sizeof(char *));
	for (i = 0; i < MAXNTOKENS; i++)
		shell_argv[i] = malloc((MAXTOKENLEN+1) * sizeof(char));

	if (!batchmode) {
		fprintf(stderr, "Welcome to Nissy "VERSION".\n"
				"Type \"commands\" for a list of commands.\n");
	}

	while (true) {
		if (!batchmode) {
			fprintf(stdout, "nissy-# ");
		}

		if (fgets(line, MAXLINELEN, stdin) == NULL)
			break;

		if (batchmode) {
			printf(">>>\n"
			       ">>> Executing command: %s"
			       ">>>\n", line);
		}

		shell_argc = parseline(line, shell_argv);

		if (shell_argc > 0)
			exec_args(shell_argc, shell_argv);

		fflush(stdout);
	}

	for (i = 0; i < MAXNTOKENS; i++)
		free(shell_argv[i]);
	free(shell_argv);
}

int
main(int argc, char *argv[])
{
	char *closing_cmd[1] = { "freemem" };

	init_env();

	if (!checkfiles()) {
		fprintf(stderr,
			"--- Warning ---\n"
			"Some pruning tables are missing or unreadable\n"
			"You can generate them with `nissy gen'.\n"
			"---------------\n\n"
		);
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "-b")) {
			launch(true);
		} else {
			exec_args(argc-1, &argv[1]);
		}
	} else {
		launch(false);
	}

	exec_args(1, closing_cmd);

	return 0;
}
