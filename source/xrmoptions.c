/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball  Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xresource.h>
#include "rofi.h"
#include "xrmoptions.h"
#include "helper.h"

typedef struct
{
    int        type;
    const char * name;
    union
    {
        unsigned int * num;
        int          * snum;
        char         ** str;
        void         *pointer;
        char         * charc;
    };
    char *mem;
} XrmOption;
/**
 * Map X resource and commandline options to internal options
 * Currently supports string, boolean and number (signed and unsigned).
 */
static XrmOption xrmOptions[] = {
    { xrm_String,  "switchers",            { .str  = &config.switchers             }, NULL },
    { xrm_Number,  "opacity",              { .num  = &config.window_opacity        }, NULL },

    { xrm_SNumber, "width",                { .snum = &config.menu_width            }, NULL },

    { xrm_Number,  "lines",                { .num  = &config.menu_lines            }, NULL },
    { xrm_Number,  "columns",              { .num  = &config.menu_columns          }, NULL },

    { xrm_String,  "font",                 { .str  = &config.menu_font             }, NULL },
    /* Foreground color */
    { xrm_String,  "foreground",           { .str  = &config.menu_fg               }, NULL },
    { xrm_String,  "fg",                   { .str  = &config.menu_fg               }, NULL },

    { xrm_String,  "background",           { .str  = &config.menu_bg               }, NULL },
    { xrm_String,  "bg",                   { .str  = &config.menu_bg               }, NULL },
    { xrm_String,  "background-alternate", { .str  = &config.menu_bg_alt           }, NULL },
    { xrm_String,  "bgalt",                { .str  = &config.menu_bg_alt           }, NULL },

    { xrm_String,  "highlightfg",          { .str  = &config.menu_hlfg             }, NULL },
    { xrm_String,  "hlfg",                 { .str  = &config.menu_hlfg             }, NULL },

    { xrm_String,  "highlightbg",          { .str  = &config.menu_hlbg             }, NULL },
    { xrm_String,  "hlbg",                 { .str  = &config.menu_hlbg             }, NULL },

    { xrm_String,  "bordercolor",          { .str  = &config.menu_bc               }, NULL },
    { xrm_String,  "bc",                   { .str  = &config.menu_bc               }, NULL },

    { xrm_Number,  "borderwidth",          { .num  = &config.menu_bw               }, NULL },
    { xrm_Number,  "bw",                   { .num  = &config.menu_bw               }, NULL },

    { xrm_Number,  "location",             { .num  = &config.location              }, NULL },

    { xrm_Number,  "padding",              { .num  = &config.padding               }, NULL },
    { xrm_SNumber, "yoffset",              { .snum = &config.y_offset              }, NULL },
    { xrm_SNumber, "xoffset",              { .snum = &config.x_offset              }, NULL },
    { xrm_Boolean, "fixed-num-lines",      { .num  = &config.fixed_num_lines       }, NULL },

    { xrm_String,  "terminal",             { .str  = &config.terminal_emulator     }, NULL },
    { xrm_String,  "ssh-client",           { .str  = &config.ssh_client            }, NULL },
    { xrm_String,  "ssh-command",          { .str  = &config.ssh_command           }, NULL },
    { xrm_String,  "run-command",          { .str  = &config.run_command           }, NULL },
    { xrm_String,  "run-list-command",     { .str  = &config.run_list_command      }, NULL },
    { xrm_String,  "run-shell-command",    { .str  = &config.run_shell_command     }, NULL },

    { xrm_Boolean, "disable-history",      { .num  = &config.disable_history       }, NULL },
    { xrm_Boolean, "levenshtein-sort",     { .num  = &config.levenshtein_sort      }, NULL },
    { xrm_Boolean, "case-sensitive",       { .num  = &config.case_sensitive        }, NULL },
    { xrm_Boolean, "sidebar-mode",         { .num  = &config.sidebar_mode          }, NULL },
    { xrm_Number,  "lazy-filter-limit",    { .num  = &config.lazy_filter_limit     }, NULL },
    { xrm_SNumber, "eh",                   { .snum = &config.element_height        }, NULL }
};

// Dynamic options.
XrmOption    *extra_options    = NULL;
unsigned int num_extra_options = 0;

void config_parser_add_option ( XrmOptionType type, const char *key, void **value )
{
    extra_options = g_realloc ( extra_options, ( num_extra_options + 1 ) * sizeof ( XrmOption ) );

    extra_options[num_extra_options].type    = type;
    extra_options[num_extra_options].name    = key;
    extra_options[num_extra_options].pointer = value;
    if ( type == xrm_String ) {
        extra_options[num_extra_options].mem = ( (char *) ( *value ) );
    }
    else {
        extra_options[num_extra_options].mem = NULL;
    }

    num_extra_options++;
}

static void config_parser_set ( XrmOption *option, XrmValue *xrmValue )
{
    if ( option->type == xrm_String ) {
        if ( ( option )->mem != NULL ) {
            g_free ( option->mem );
            option->mem = NULL;
        }
        *( option->str ) = g_strndup ( xrmValue->addr, xrmValue->size );

        // Memory
        ( option )->mem = *( option->str );
    }
    else if ( option->type == xrm_Number ) {
        *( option->num ) = (unsigned int) strtoul ( xrmValue->addr, NULL, 10 );
    }
    else if ( option->type == xrm_SNumber ) {
        *( option->snum ) = (int) strtol ( xrmValue->addr, NULL, 10 );
    }
    else if ( option->type == xrm_Boolean ) {
        if ( xrmValue->size > 0 && g_ascii_strncasecmp ( xrmValue->addr, "true", xrmValue->size ) == 0 ) {
            *( option->num ) = TRUE;
        }
        else{
            *( option->num ) = FALSE;
        }
    }
    else if ( option->type == xrm_Char ) {
        *( option->charc ) = helper_parse_char ( xrmValue->addr );
    }
}

void config_parse_xresource_options ( Display *display )
{
    char *xRMS;
    // Map Xresource entries to rofi config options.
    XrmInitialize ();
    xRMS = XResourceManagerString ( display );

    if ( xRMS == NULL ) {
        return;
    }
    XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

    char        * xrmType;
    XrmValue    xrmValue;
    const char  * namePrefix  = "rofi";
    const char  * classPrefix = "rofi";

    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( XrmOption ); ++i ) {
        char *name, *class;

        name  = g_strdup_printf ( "%s.%s", namePrefix, xrmOptions[i].name );
        class = g_strdup_printf ( "%s.%s", classPrefix, xrmOptions[i].name );

        if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) ) {
            config_parser_set ( &( xrmOptions[i] ), &xrmValue );
        }

        g_free ( class );
        g_free ( name );
    }
    XrmDestroyDatabase ( xDB );
}

/**
 * Parse an option from the commandline vector.
 */
static void config_parse_cmd_option ( XrmOption *option, int argc, char **argv )
{
    // Prepend a - to the option name.
    char *key = g_strdup_printf ( "-%s", option->name );
    switch ( option->type )
    {
    case xrm_Number:
        find_arg_uint ( argc, argv, key, option->num );
        break;
    case xrm_SNumber:
        find_arg_int ( argc, argv, key, option->snum );
        break;
    case xrm_String:
        if ( find_arg_str ( argc, argv, key, option->str ) == TRUE ) {
            if ( option->mem != NULL ) {
                g_free ( option->mem );
                option->mem = NULL;
            }
        }
        break;
    case xrm_Boolean:
        if ( find_arg ( argc, argv, key ) >= 0 ) {
            *( option->num ) = TRUE;
        }
        break;
    case xrm_Char:
        find_arg_char ( argc, argv, key, option->charc );
        break;
    default:
        break;
    }
    g_free ( key );
}

void config_parse_cmd_options ( int argc, char ** argv )
{
    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( XrmOption ); ++i ) {
        XrmOption *op = &( xrmOptions[i] );
        config_parse_cmd_option ( op, argc, argv );
    }
}

void config_parse_cmd_options_dynamic ( int argc, char ** argv )
{
    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        XrmOption *op = &( extra_options[i] );
        config_parse_cmd_option ( op, argc, argv );
    }
}

void config_parse_xresource_options_dynamic ( Display *display )
{
    char *xRMS;
    // Map Xresource entries to rofi config options.
    XrmInitialize ();
    xRMS = XResourceManagerString ( display );

    if ( xRMS == NULL ) {
        return;
    }
    XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

    char        * xrmType;
    XrmValue    xrmValue;
    const char  * namePrefix  = "rofi";
    const char  * classPrefix = "rofi";

    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        char *name, *class;

        name  = g_strdup_printf ( "%s.%s", namePrefix, extra_options[i].name );
        class = g_strdup_printf ( "%s.%s", classPrefix, extra_options[i].name );
        if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) ) {
            config_parser_set ( &( extra_options[i] ), &xrmValue );
        }

        g_free ( class );
        g_free ( name );
    }
    XrmDestroyDatabase ( xDB );
}

void config_xresource_free ( void )
{
    for ( unsigned int i = 0; i < ( sizeof ( xrmOptions ) / sizeof ( *xrmOptions ) ); ++i ) {
        if ( xrmOptions[i].mem != NULL ) {
            g_free ( xrmOptions[i].mem );
            xrmOptions[i].mem = NULL;
        }
    }
    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        if ( extra_options[i].mem != NULL ) {
            g_free ( extra_options[i].mem );
            extra_options[i].mem = NULL;
        }
    }
    if ( extra_options != NULL ) {
        g_free ( extra_options );
    }
}

static void xresource_dump_entry ( const char *namePrefix, XrmOption *option )
{
    printf ( "%s.%s: %*s", namePrefix, option->name,
             (int) ( 20 - strlen ( option->name ) ), "" );
    switch ( option->type )
    {
    case xrm_Number:
        printf ( "%u", *( option->num ) );
        break;
    case xrm_SNumber:
        printf ( "%i", *( option->snum ) );
        break;
    case xrm_String:
        if ( ( *( option->str ) ) != NULL ) {
            printf ( "%s", *( option->str ) );
        }
        break;
    case xrm_Boolean:
        printf ( "%s", ( *( option->num ) == TRUE ) ? "true" : "false" );
        break;
    case xrm_Char:
        if ( *( option->charc ) > 32 && *( option->charc ) < 127 ) {
            printf ( "%c", *( option->charc ) );
        }
        else {
            printf ( "\\x%02X", *( option->charc ) );
        }
        break;
    default:
        break;
    }
    printf ( "\n" );
}

void xresource_dump ( void )
{
    const char   * namePrefix = "rofi";
    unsigned int entries      = sizeof ( xrmOptions ) / sizeof ( *xrmOptions );
    for ( unsigned int i = 0; i < entries; ++i ) {
        // Skip duplicates.
        if ( ( i + 1 ) < entries ) {
            if ( xrmOptions[i].str == xrmOptions[i + 1].str ) {
                continue;
            }
        }
        xresource_dump_entry ( namePrefix, &( xrmOptions[i] ) );
    }
    for ( unsigned int i = 0; i < num_extra_options; i++ ) {
        xresource_dump_entry ( namePrefix, &( extra_options[i] ) );
    }
}
