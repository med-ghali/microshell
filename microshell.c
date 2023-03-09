#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct commands{
	char **opt;
	struct commands *next;
}commands;

int ft_strlen(char *s)
{
	int i = 0;
	while(s[i])
		i++;
	return i;
}

void ft_putstr(char *s)
{
	write(2,s,ft_strlen(s));
}

void cd_bad_args()
{
	ft_putstr("error: cd: bad arguments\n");
}

void cd_bad_path(char *path)
{
	ft_putstr("error: cd: cannot change directory to ");
	ft_putstr(path);
	ft_putstr("\n");
}

void syscall_error()
{
	ft_putstr("error: fatal\n");
	exit(1);
}

void execve_error(char *cmd)
{
	ft_putstr("error: cannot execute ");
	ft_putstr(cmd);
	ft_putstr("\n");
	exit(1);
}

char **next_cmd(char **av)
{
	int f = 0;
	while (*av)
	{
		while(*av && !strcmp(*av,";"))
		{
			f =1;
			*av = 0;
			av ++;
		}
		if (f)
			return av;
		av++;
	}
	return av;
	
}
commands *new_node(char **av)
{
	commands *new = malloc(sizeof(commands));
	if (!new)
		syscall_error();
	new->opt = av;
	new->next = 0;
	return new;
}
void add_back(commands *cmd, char **av)
{
	while(cmd->next)
		cmd = cmd->next;
	cmd->next = new_node(av);
}

char **parse(commands **cmds,char **av)
{
	commands * cmd = 0;
	while(*av && !strcmp(*av,";"))
		av++;
	if (!(*av))
	{
		*cmds = 0;
		return NULL;
	}
	char **next = next_cmd(av);
	cmd = new_node(av);
	while(*av)
	{
		if (!strcmp(*av,"|"))
		{
			*av = 0;
			av ++;
			add_back(cmd,av);
		}
		else
			av++;
	}
	*cmds = cmd;
	return next;
}


void ft_chdir(commands *cmd)
{
	if (!cmd->opt[1])
	{
		cd_bad_args();
		return ;
	}
	if (chdir(cmd->opt[1]) == -1 )
		cd_bad_path(cmd->opt[1]);
}

void exec_single_cmd(commands *cmds, char **envp)
{
	int pid;
	pid = fork();
	if (pid == -1)
		syscall_error();
	if (pid ==0 )
	{
		if (!strcmp(cmds->opt[0],"cd"))
		{
			ft_chdir(cmds);
			exit(1);
		}
		if (execve(cmds->opt[0],cmds->opt,envp) == -1)
			execve_error(cmds->opt[0]);
		exit(1);
	}
}

void execute(commands *cmds, char **envp)
{
	int tmp_in = dup(0);
	int pid;
	if (tmp_in == -1)
		syscall_error();
	int fds[2];
	while(cmds)
	{
		if (!cmds->next)
			exec_single_cmd(cmds,envp);
		else
		{
			if (pipe(fds) == -1)
				syscall_error();
			pid = fork();
			if (pid == -1)
				syscall_error();
			if (pid ==0 )
			{
				dup2(fds[1],1);
				close(fds[1]);
				close(fds[0]);
				if (!strcmp(cmds->opt[0],"cd"))
				{
					ft_chdir(cmds);
					exit(1);
				}
				if (execve(cmds->opt[0],cmds->opt,envp) == -1)
					execve_error(cmds->opt[0]);
				exit(1);
			}
			else
			{
				dup2(fds[0],0);
				close(fds[1]);
				close(fds[0]);
			}	
		}
		cmds = cmds->next;
	}
	if (dup2(tmp_in,0) == -1)
		syscall_error();
	close(tmp_in);
}
void execute_one(commands *cmds, char **envp)
{
	if (!strcmp(cmds->opt[0],"cd"))
	{
		ft_chdir(cmds);
		return ;
	}
	int pid;
	pid = fork();
	if (pid == -1)
		syscall_error();
	if (pid ==0 )
	{
		if (execve(cmds->opt[0],cmds->opt,envp) == -1)
			execve_error(cmds->opt[0]);
		exit(1);
	}
}

void exec(commands *cmds, char **envp)
{
	if (cmds->next)
		execute(cmds,envp);
	else
		execute_one(cmds,envp);
	
	while(waitpid(-1,NULL,0) != -1)
		;
}

void free_all(commands *cmds)
{
	commands *tmp;
	while (cmds)
	{
		tmp = cmds;
		cmds = cmds->next;
		free(tmp);
	}
	
}

int main(int ac,char *av[],char *envp[])
{
	commands *cmd = NULL;
	if (ac < 2)
		return 0;
	av++;
	while(av && *av)
	{
		av = parse(&cmd,av);
		if (cmd)
		{
			exec(cmd,envp);
			free_all(cmd);
		}
	}
	return 0;
	
}