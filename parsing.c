/*
MySecureShell permit to add restriction to modified sftp-server
when using MySecureShell as shell.
Copyright (C) 2007 Sebastien Tardif

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation (version 2)

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "config.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "string.h"
#include "parsing.h"

int	parse_opened_tag = 0;

void	parse_tag(char *buffer)
{
  char	*str;
  int	len, is_close_tag = 0;

  str = trim_left(buffer + 1);
  if (*str == '/')
    {
      is_close_tag = 1;
      str = trim_left(str + 1);
    }
  len = *str != '\0' ? strlen(str) - 1 : 0;
  str[len] = '\0';
  str = trim_right(str);
  if (is_close_tag == 1)
    parse_tag_close(str);
  else
    parse_tag_open(str);
}

void	parse_tag_close(const char *str)
{
  if (strcasecmp(str, TAG_GROUP) == 0)
      hash_set("GROUP", 0);
  else if (strcasecmp(str, TAG_USER) == 0)
      hash_set("USER", 0);
  else if (strcasecmp(str, TAG_RANGEIP) == 0)
    hash_set("RANGEIP", 0);
  else if (strcasecmp(str, TAG_VIRTUALHOST) == 0)
  {
  	hash_set("VIRTUALHOST_IP", 0);
	hash_set_int("VIRTUALHOST_PORT", 0);
  }
  else if (strcasecmp(str, TAG_DEFAULT) == 0)
    hash_set_int("DEFAULT", 0);
  parse_opened_tag--;
}

void	parse_tag_open(char *str)
{
  char	*s;

  if ((s = strchr(str, ' ')) != NULL || (s = strchr(str, '\t')) != NULL)
      {
	*s = '\0';
	s = trim_left(s + 1);
      }
  str = trim_right(str);
  if (strcasecmp(str, TAG_GROUP) == 0)
    hash_set("GROUP", strdup(s));
  else if (strcasecmp(str, TAG_USER) == 0)
    hash_set("USER", strdup(s));
  else if (strcasecmp(str, TAG_RANGEIP) == 0)
    hash_set("RANGEIP", parse_range_ip(s));
  else if (strcasecmp(str, TAG_VIRTUALHOST) == 0)
    parse_virtualhost(s);
  else if (strcasecmp(str, TAG_DEFAULT) == 0)
    hash_set_int("DEFAULT", 1);
  parse_opened_tag++;
}

void			parse_virtualhost(const char *str)
{
  struct hostent	*h;
  char			*ptr;
  int			port = 0;
	
  if ((ptr = strchr(str, ':')))
    {
      *ptr = '\0';
      port = atoi(ptr + 1);
    }
  if (!(str[0] == '*' && str[1] == '\0') &&
      (h = gethostbyname(str)) != NULL &&
      h->h_addr_list != NULL && h->h_addr_list[0] != NULL)
    {
      char	buffer[32];
      
      (void )snprintf(buffer, sizeof(buffer), "%ui.%ui.%ui.%ui",
		      (unsigned int)h->h_addr_list[0][0],
		      (unsigned int)h->h_addr_list[0][1],
		      (unsigned int)h->h_addr_list[0][2],
		      (unsigned int)h->h_addr_list[0][3]
		      );
      hash_set("VIRTUALHOST_IP", strdup(buffer));
    }
  else
    hash_set("VIRTUALHOST_IP", strdup(str));
  hash_set_int("VIRTUALHOST_PORT", port);
}

char	*parse_range_ip(const char *str)
{
  char	*mask = calloc(10, sizeof(char));
  int	i, nb, pos;

  mask[8] = (char )32;
  for (i = 0, nb = 0, pos = 0; str[i] != '\0'; i++)
    if (str[i] >= '0' && str[i] <= '9')
      nb = nb * 10 + ((int )str[i] - (int )'0');
    else if ((str[i] == '.' || str[i] == '-' || str[i] == '/') && pos <= 7)
      {
	mask[pos] = (char )nb;
	if (pos >= 0 && pos <= 3)
	  mask[pos + 4] = (char )nb;
	pos++;
	nb = 0;
      }
  mask[pos] = (char )nb;
  if (pos >= 0 && pos <= 3)
    mask[pos + 4] = (char )nb;
  if (mask[8] > (char )32)
    mask[8] = (char )32;
  return (mask);
}

char	**parse_cut_string(char *str)
{
  char	**tb = NULL;
  char	*word = NULL;
  int	nb = 0;

  while (*str != '\0')
    {
      if (*str == ' ' || *str == '\t')
	{
	  *str = '\0';
	  if (word != NULL)
	    {
	      word = clean_string(word);
	      if (*word != '\0' && strcmp(word, "=") != 0)
		{
		  tb = realloc(tb, (nb + 2) * sizeof(*tb));
		  tb[nb++] = word;
		  tb[nb] = 0;
		}
	      word = NULL;
	    }
	}
      else
	{
	  if (word == NULL)
	    word = str;
	  if (*str == '\'' || *str == '"')
	    {
	      char  c = *str;

	      str++;
	      while (c != *str && *str != '\0')
		str++;
	    }
	  else if (*str == '\\')
	    str++;
	}
      str++;
    }
  if (word != NULL)
    {
      word = clean_string(word);
      if (*word != '\0' && strcmp(word, "=") != 0)
	{
	  tb = realloc(tb, (nb + 2) * sizeof(*tb));
	  tb[nb++] = word;
	  tb[nb] = 0;
	}
    }
  return (tb);
}
