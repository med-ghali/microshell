#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void syscall_error();
typedef struct cmds {
	char **opt;
	struct cmds *next;
} cmds;

int ft_strlen(char *s)
{
	int i =0;
	while(s[i])
		i++;
	return (i);
}

void ft_putstr(char *s,int fd)
{
	int check = write(fd,s,ft_strlen(s));
	if (check == -1)
		syscall_error();
}

void syscall_error()
{
	ft_putstr("error: fatal\n",2);
	exit(1);
}

void execve_error(char *cmd)
{
	ft_putstr("error: cannot execute ",2);
	ft_putstr(cmd,2);
	ft_putstr("\n",2);
	exit(1);
}

void cd_wrong_argument()
{
	ft_putstr("error: cd: bad arguments\n",2);
}

void cd_failed(char *path)
{
	ft_putstr("error: cd: cannot change directory to ",2);
	ft_putstr(path,2);
	ft_putstr("\n",2);
}

char **next_cmd(char **av)
{
	int f = 0;

	while(*av)
	{
		while (*av && !strcmp(*av,";"))
		{
			*av = NULL;
			av ++;
			f = 1;
		}
		if (f)
			return av;
		av++;
	}
	return (av);
}

cmds *new_node(char **av)
{
	cmds *new = malloc(sizeof(cmds));
	if (!new)
		syscall_error();
	new->opt = av;
	new->next = 0;
	return new;
}

void add_back(cmds *cmd,char **av)
{
	cmds *new = new_node(av);
	while(cmd->next)
		cmd = cmd->next;
	cmd->next = new;
}

char **parse(cmds **cmd,char **av)
{
	cmds *new = 0 ;
	while(*av &&!strcmp(*av,";"))
		av++;
	if (*av == NULL)
		return NULL;
	char **next = next_cmd(av);
	new = new_node(av);
	while(*av)
	{
		if (!strcmp(*av,"|"))
		{
			*av = NULL;
			av++;
			add_back(new,av);
		}
		else
			av++;
	}
	*cmd = new;
	return next;
}

void print_list(cmds *cmd)
{
	while(cmd)
	{
		int i = 0;
		printf("next node : ------------- \n");
		while(cmd->opt[i])
		{
			printf("%s\n",cmd->opt[i]);
			i++;
		}
		cmd = cmd->next;
	}
}

void ft_chdir(cmds *cmd)
{
	int ret;
	if (cmd->opt[1])
	{
		ret =chdir(cmd->opt[1]);
		if (ret == -1)
			cd_failed(cmd->opt[1]);
	}
	else
		cd_wrong_argument();
}

void exec_single_cmd(cmds *cmd,char **envp)
{
	if(!strcmp(cmd->opt[0],"cd"))
		ft_chdir(cmd);
	else
	{
		int pid = fork();
		if (pid == 0)
		{
			if (execve(cmd->opt[0],cmd->opt,envp) == -1)
				execve_error(cmd->opt[0]);
		}
		else
			return ;
	}
}

void exec_cmds(cmds *cmd,char **envp)
{
	int tmp_in = dup(0);
	int fds[2];
	int pid;

	while(cmd)
	{
		if (cmd->next)
			pipe(fds);
		if (!cmd->next)
			exec_single_cmd(cmd,envp);
		else
		{
			pid = fork();
			if (pid == -1)
				syscall_error();
			if (pid == 0)
			{
				close(fds[0]);
				dup2(fds[1],1);
				close(fds[1]);
				if (execve(cmd->opt[0],cmd->opt,envp) == -1)
					execve_error(cmd->opt[0]);
			}
			else
			{
				dup2(fds[0],0);
				close(fds[0]);
				close(fds[1]);
			}
		}
		cmd = cmd->next;
	}
	dup2(tmp_in,0);
	while(waitpid(-1,NULL,0) > 0)
		;
}

void execute(cmds *cmd,char **envp)
{
	if (!cmd->next)
	{
		exec_single_cmd(cmd,envp);
		return ;
	}
	else
		exec_cmds(cmd,envp);
}

int main(int ac, char *av[], char *envp[])
{
	cmds *cmd;
	if (ac < 2)
		return 0;
	av++;
	while (*av)
	{
		av = parse(&cmd,av);
		if (cmd)
			execute(cmd,envp);
		else
			break;
	}
}