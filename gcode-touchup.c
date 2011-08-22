// Play with gcode from skeinforge

#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <err.h>

#define lift 0.4
#define	rate 100


int
main (int argc, const char *argv[])
{
  char c;
  int debug = 0;
  int invert = 0;

  poptContext optCon;		// context for parsing command-line options
  const struct poptOption optionsTable[] = {
//      {"string", 's', POPT_ARG_STRING, &string, 0, "String", "string"},
//      {"string-default", 'S', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &string, 0, "String", "string"},
    {"invert", 'i', POPT_ARG_NONE, &invert, 0, "Invert", 0},
    {"debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug", 0},
    POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
  };

  optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
  //poptSetOtherOptionHelp (optCon, "");

  if ((c = poptGetNextOpt (optCon)) < -1)
    errx (1, "%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

  if (poptPeekArg (optCon))
    {
      poptPrintUsage (optCon, stderr, 0);
      return -1;
    }

  printf ("G21             ; metric\n"	//
	  "G90             ; absolute\n"	//
	  "G92 Z0 E0       ; don't move E or Z\n"	//
	  "M106 	   ; fan on\n"	//
    );

  char line[1000];
  char *o[26] = { };
  char hop[100] = "";
  while (fgets (line, sizeof (line), stdin))
    {
      char *p = line + strlen (line);
      while (p > line && p[-1] < ' ')
	p--;
      *p = 0;
      if (!strncmp (line, "M104", 4))
	{
	  if (o['Z' - 'A'] && o['F' - 'A'])
	    {
	      double hi = atof (o['Z' - 'A']) + lift;
	      char u[100];
	      sprintf (u, "%.2f", hi);
	      printf ("G1 Z%s F%u; hop\n", u, rate);
	    }
	  memcpy (line, "M109", 4);	// temp wait not just set
	  if (o['Z' - 'A'] && o['F' - 'A'])
	    snprintf (hop, sizeof (hop), "G1 Z%s F%u; back\nG1 F%s\n", o['Z' - 'A'], rate, o['F' - 'A']);
	}
      else if (!strncmp (line, "G1 ", 3))
	{
	  int n;
	  char *l[26] = { };
	  p = line;
	  while (*p)
	    {
	      if (isalpha (*p))
		{
		  n = toupper (*p) - 'A';
		  char *q = p;
		  while (*q && !isspace (*q))
		    q++;
		  if (l[n])
		    free (l[n]);
		  l[n] = strndup (p + 1, q - p - 1);
		}
	      p++;
	    }
	  if (invert && *line && l['X' - 'A'])
	    {			// invert X
	      char u[100];
	      sprintf (u, "%.2f", 200.0 - atof (l['X' - 'A']));
	      free (l['X' - 'A']);
	      l['X' - 'A'] = strdup (u);
	    }
	  if ((o['Z' - 'A'] && l['Z' - 'A'] && (l['X' - 'A'] || l['Y' - 'A']) && strcmp (o['Z' - 'A'], l['Z' - 'A'])) || (o['E' - 'A'] && o['Z' - 'A'] && l['X' - 'A'] && l['Y' - 'A'] && o['X' - 'A'] && o['Y' - 'A'] && (strcmp (l['X' - 'A'], o['X' - 'A']) || strcmp (l['Y' - 'A'], o['Y' - 'A'])) &&	//
															  (!l['E' - 'A']
															   || !strcmp (o['E' - 'A'],
																       l['E' - 'A']))))
	    {			// hop
	      double hi = atof (o['Z' - 'A']) + lift;
	      if (l['Z' - 'A'])
		{
		  double target = atof (l['Z' - 'A']);
		  if (target > hi)
		    hi = target;
		}
	      char u[100];
	      sprintf (u, "%.2f", hi);
	      printf ("G1 Z%s F%u	; hop\n", u, rate);
	      printf ("G1");
	      for (n = 0; n < 26; n++)
		if (n != 'Z' - 'A' && l[n])
		  printf (" %c%s", n + 'A', l[n]);
	      if (!l['F' - 'A'] && o['F' - 'A'])
		printf (" F%s", o['F' - 'A']);
	      printf ("\n");
	      *line = 0;
	      snprintf (hop, sizeof (hop), "G1 Z%s F%u; back\nG1 F%s\n", l['Z' - 'A'] ? : o['Z' - 'A'], rate, l['F' - 'A'] ? : o['F' - 'A']);
	    }
	  else if (*hop)
	    {
	      printf ("%s", hop);
	      *hop = 0;
	    }
	  if (invert && *line && l['X' - 'A'])
	    {			// needs printing from data as X was inverted
	      printf ("G1");
	      for (n = 0; n < 26; n++)
		if (l[n])
		  printf (" %c%s", n + 'A', l[n]);
	      printf ("\n");
	      *line = 0;
	    }
	  for (n = 0; n < 26; n++)
	    if (l[n])
	      {
		if (o[n])
		  free (o[n]);
		o[n] = l[n];
		l[n] = NULL;
	      }
	}
      if (*line)
	printf ("%s\n", line);
    }

  printf ("G92 Z0 E0	   ; reset Z/E\n"	//
	  "M108 S0         ; Cold hot end\n"	//
	  "M140 S0         ; Cold bed\n"	//
	  "G1 Z10 F100     ; up\n"	//
	  "G1 X0 Y0 F2300  ; to home\n"	//
	  "G1 E10 F20      ; feed a bit while cooling\n"	//
	  "M084            ; Disable steppers\n"	//
	  "M107            ; fan off\n"	//
    );

  return 0;
}
