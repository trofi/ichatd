#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"

#include "globals.h"
#include "options.h"
#include "getword.h"

// IMPLEMENTS:
// parse_options
// parse_option
// get_options

/*
  int analyze_cmd( char *cmd, int cmdlen )
  {
  int i, vall = 0;
  char opt[256], value[256];
  int val;

  memset( opt, 32, 256 );
  memset( value, 32, 256 );

  opt[255] = '\0';
  value[0] = '\0';
  value[255] = '\0';

  for( i = 0; i < cmdlen; i++ )
  {
  if( (cmd[i] == '=') || (cmd[i] == ' ') || (cmd[i] == '\t') ) break;
    
  opt[i] = cmd[i];
  }
    
  opt[i] = '\0';

  if ( i == cmdlen )
  {

  if ( !g_restarted ) printf("Error in config file: value not given for parameter %s\n", opt);
  return(1);
  }
    
  for( ; i < cmdlen; i++ )
  {

  if( (cmd[i] != '=') && (cmd[i] != ' ') && (cmd[i] != '\t') )
  { 
  for ( vall = 0; (i < cmdlen) && (vall < 255) ; i++, vall++ )
  {
  if ( (cmd[i] == '=') || (cmd[i] == ' ') || (cmd[i] == '\t') ) break;
  value[vall] = cmd[i];
  }
  break;
  }
  }
    
  value[vall] = '\0';
    
  if ( value[0] == '\0' )
  {
  if ( !g_restarted ) printf("Error in config file: value not given for parameter %s\n", opt);
  return(1);
  }
    

  //    printf("OPTION %s\nVALUE %s\n", opt, value );

  if( !strcasecmp( opt, "logfile" ) )
  {


  val = strlen( value );
    
  if( (val < 1) && (val > 160) )
  {
  if ( !g_restarted ) printf(" Invalid logfile value: %s. Length of file path must be less than 160 \n", value);
  return(1);
  }
    
  strcpy( g_logfile, (char *)value );

  if ( !g_restarted ) printf("  LOGFILE: %s\n", g_logfile );
    
  return (0);
  }

  if( !strcasecmp( opt, "pidfile" ) )
  {


  val = strlen( value );
    
  if( (val < 1) && (val > 160) )
  {
  if ( !g_restarted ) printf(" Invalid pidfile value: %s. Length of file path must be less than 160 \n", value);
  return(1);
  }
    
  strcpy ( g_pidfile, (char *)value );

  if ( !g_restarted ) printf("  PIDFILE: %s\n", g_pidfile );
    
  return (0);
  }*/
    

/*    if( !strcasecmp( opt, "MaxClients" ) )
      {

      val = atoi( value );
    
      if( (val < 1) && (val > 4000) )
      {
      if ( !g_restarted ) printf(" Invalid maxclient value: %s. Must be from 1 to 4000\n", value);
      return(1);
      }
    
      MAXCLIENTS = val;

      if ( !g_restarted ) printf("  Maxclients = %d\n", MAXCLIENTS );
    
      return (0);
      }*/


/*    if( !strcasecmp( opt, "PORT" ) )
      {
      val = atoi( value );
    
      if ( (val < 1) && ( val > 65525 ) )
      {
      if ( !g_restarted ) printf(" Invalid PORT setting %s. PORT must be from 1 to 65535\n", value);
      return(1);
      }

      g_userport = val;

      if ( !g_restarted ) printf("  PORT %d\n", g_userport);

      return (0);

      }
    
      if ( !g_restarted )
      printf(" Unknown option %s\n", opt );
    
      return(0);    
      }

      int get_options_old()
      {
      FILE *optfile;
      int c, i, t;
      char string[256];
      char cmd[256];
      int cmdl;
      int g_need2exit = 0;
      int newlinep;
      FILE* f;

      // test code...
      f = fopen( "./dump.txt", "wt" );
      optfile = fopen( OPT_FILE, "rt" );
    
      while( getword( optfile, string, & newlinep, OPT_FILE ) )
      {
      fprintf( f, "WORD: '%s'\tnewline: %i\n", string, newlinep );
      }
    
      fclose( f );

      exit( 1 );

      //
      cmd[255] = '\0';    

      strcpy( g_logfile, DEF_LOGFILE );
      strcpy( g_pidfile, DEF_PIDFILE );

      optfile = fopen( OPT_FILE, "r" );
    

      if( optfile == NULL )
      {
      perror("Can't open config file: ");   
      if( !g_restarted ) printf("Using default options\n");

      //    MAXCLIENTS = DEF_MAXCLIENTS;
      g_userport = DEF_USERPORT;
      strcpy( g_logfile, DEF_LOGFILE );
      return(0);
      }

    
      for(i = 0 ;; i++)
      {
      c = getc( optfile );
      if( c == EOF ) break;
    
      string[i] = c;

      if(c == '\n')
      {
        

      if ( i > 0 )
      { 
      //        printf("STRING %s\n", string);


      for( t = 0; t < i; t++ )
      {
      if( (string[t] == '#') || (string[t] == ';') )
      {
      i = -1;
      break;
        
      }

      //            printf(" I = %d\n", i );
        
      if( (string[t] != ' ') && (string[t] != '\t') )
      {
      memset( cmd, 32, 256 );
        
      for( cmdl = 0; ( t < i ) && (cmdl < 255 ); t++, cmdl++ ) cmd[cmdl] = string[t];
      //            printf("CMD %s\n\n", cmd);
      if ( analyze_cmd( cmd, cmdl ) ) g_need2exit = 1;
      i = -1;
      break;
            
      }
      }
      }
      else i = -1;    

      }
    
      if( i >= 256 ) i = -1;

    
    
    
      }

      fclose( optfile );    

      if ( g_need2exit ) return(1);
      return(0);

      }*/

// return 0 - error, 1 - success
static int
parse_option (FILE * optfile, char * word, int * newlinep)
{
    if (strcasecmp (word, "port") == 0
        || strcasecmp (word, "userport") == 0)
    {
        if (getword (optfile, word, newlinep, DEF_CONFIG_FILE) != 1
            || *newlinep)
        {
            fprintf (stderr, "ERROR: Invalid parameter for 'userport' option\n");
            return 0;
        }

        g_userport = atoi (word);
    }
    else if (strcasecmp( word, "serverport") == 0)
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'serverport' option\n" );
            return 0;
        }
    
        g_serverport = atoi( word );
    }
    else if( strcasecmp( word, "reuseaddress" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'reuseaddress' option\n" );
            return 0;
        }
    
        g_reuseaddr = ( atoi( word ) ) ? 1 : 0;
    }
    else if( strcasecmp( word, "logfile" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'logfile' option\n" );
            return 0;
        }
    
        strcpy( g_logfile, word );
    }
    else if( strcasecmp( word, "loglevel" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'logfile' option\n" );
            return 0;
        }
        g_log_level = atoi( word );
    }
    else if( strcasecmp( word, "pidfile" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'pidfile' option\n" );
            return 0;
        }
    
        strcpy( g_pidfile, word );
    }
    else if( strcasecmp( word, "logmessages" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'logmessages' option\n" );
            return 0;
        }
    
        g_logmessages = ( atoi( word ) ) ? 1 : 0;
    }
    else if( strcasecmp( word, "serverloops" ) == 0 )
    {
    }
    else if( strcasecmp( word, "serverpassword" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'serverpassword' option\n" );
            return 0;
        }
    
        strcpy( g_servpass, word );
    }
    else if( strcasecmp( word, "servername" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'servername' option\n" );
            return 0;
        }
    
        strcpy( g_servname, word );
    }
    else if( strcasecmp( word, "nullclients" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'nullclients' option\n" );
            return 0;
        }
    
        g_enablenulluser = ( atoi( word ) ) ? 1 : 0;
    }
    else if( strcasecmp( word, "minheaderlength" ) == 0 )
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 ||* newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'minheaderlength' option\n" );
            return 0;
        }
    
        g_minhdrlen = atoi( word );
    }
    else if( strcasecmp( word, "initialmsgbufsize" ) == 0 ) 
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'initialmsgbufsize' option\n" );
            return 0;
        }
    
        g_initialmsgbufsz = atoi( word );
    }
    else if( strcasecmp( word, "maxmsgsize" ) == 0 ) 
    {
        if( getword( optfile, word, newlinep, DEF_CONFIG_FILE ) != 1 || *newlinep )
        {
            fprintf( stderr, "ERROR: Invalid parameter for 'maxmsgsize' option\n" );
            return 0;
        }
    
        g_maxmsgsz = atoi( word );
    }
    else
    {
        fprintf( stderr, "ERROR: Unknown option '%s'\n", word );
        return 0;
    }
    
    return 1;
}

int
parse_options (FILE* optfile)
{
    char word[MAXWORDLEN*2];
    int newlinep, result;

    // scan options...
    for( ;; )
    {
        // no options - no actions
        result = getword( optfile, word, & newlinep, DEF_CONFIG_FILE );
        if( result == -1 )  // error
            return 0;
        else if( result == 0 )  // no words
            return 1;
    
        // not a line beginning
        if( ! newlinep )
        {
            fprintf( stderr, "ERROR: Unexpected option '%s'\n", word );
            return 0;
        }

        // oki, check the option
        if( ! parse_option( optfile, word, & newlinep ) )
            return 0;
    }
}

// return 0 on error, 1 - success
int
get_options (void)
{
    FILE* optfile = NULL;
    int result;
    
    // setup default values...
    // assume that all numeric variables habve defaults...
    strcpy( g_logfile, DEF_LOG_FILE );
    strcpy( g_pidfile, DEF_PID_FILE );
    strcpy( g_servpass, DEF_SERVER_PASSWORD );
    g_servname[0] = 0; // default value is to be generated...

    // try to open log file
    optfile = fopen( DEF_CONFIG_FILE, "rt" );
    if( ! optfile )
    {
        fprintf( stderr, "\nERROR (non-fatal): Can't open config file '%s': %s", DEF_CONFIG_FILE, strerror( errno ) );
        return 1;
    }

    //
    result = parse_options (optfile);
    
    //
    fclose (optfile);
    
    //
    return result;
}
