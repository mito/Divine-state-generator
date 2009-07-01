#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

void bye( int x ) {
    cout << "E";
    exit( x );
}

void printout( std::string dir ) {
    // read in the output and dump it to stdout
    DIR *d = opendir( dir.c_str() );
    dirent *ent = 0;
    while ( ( ent = readdir( d ) ) != 0 ) {
        string name( ent->d_name ), line;
        if ( name == "." || name == ".." )
            continue;
        string f = dir + "/" + name;

        // skip directories
        DIR *d = opendir( f.c_str() );
        if ( d ) { closedir( d ); continue; }

        // cerr << "reading: " << f << endl;
        ifstream i( f.c_str() );
        if ( !i.is_open() )
            continue;
        while ( !getline( i, line ).eof() ) {
            cout << name << ": " << line << endl;
        }
        i.close();
    }

}

void rmtree( std::string root ) {
    DIR *d = opendir( root.c_str() );
    if ( !d ) {
        unlink( root.c_str() );
        return;
    }
    dirent *ent = 0;
    while ( ( ent = readdir( d ) ) != 0 ) {
        string name( ent->d_name );
        if ( name == "." || name == ".." )
            continue;
        rmtree( root + "/" + name );
    }
    rmdir( root.c_str() );
}

int main( int argc, char *argv[] ) {
    typedef map< string, ofstream * > Files;
    typedef vector< string > Commands;
    Commands commands;
    std::string dir;
    std::string based;
    Files files;
    int i = 0;
    char buf[512];

    cin >> based;
    if ( based != "work-directory:" )
        bye( 2 );
    cin >> based;

    cerr << "work-directory:: " << based << endl;

    // make a working directory
    // this is secure against symlink attacks because mkdir fails in
    // presence of anything with a given name
    while ( true ) {
        string templ = based + "/run.XXXXXX";
        strcpy( buf, templ.c_str() );
        char *_dir = mktemp( buf );
        if (! _dir ) {
            cerr << "problem calling mktemp" << endl;
            bye( 3 );
        }
        dir = string( _dir );
        if ( dir == "" ) {
            cerr << "could not determine tmpdir name... "
                 << "probably permission problem?" << endl;
            bye( 4 );
        }
        if ( ::mkdir( dir.c_str(), 0770 ) == 0 ) {
            // ::mkdir( ( dir + "/in" ).c_str(), 0770 );
            ::chdir( dir.c_str() );
            break;
        }
        cerr << "could not create: " << dir << endl;
        ++ i;
        if ( i > 64 ) {
            cerr << "could not create working directory" << endl;
            bye( 1 );
        }
    }

    cerr << "reading input..." << endl;
    
    string line;
    while ( !getline( cin, line ).eof() ) {
        int colon = line.find( ": " );
        if ( colon == string::npos )
            continue;
        string f = string( line, 0, colon );
        string l = string( line, colon + 2, string::npos );
        if ( f == "command" ) {
            commands.push_back( l );
        } else {
            if ( !files[ f ] )
                files[ f ] = new ofstream( (dir + "/" + f).c_str() );
            *(files[ f ]) << l << endl;
        }
    }

    for ( Files::iterator i = files.begin(); i != files.end(); ++i )
        i->second->close();

    pid_t child = ::fork();

    if ( child == 0 ) { // we are in child, run commands
        for ( Commands::iterator i = commands.begin(); i != commands.end(); ++i ) {
            cerr << "running: " << *i << endl;
            int ex = ::system( i->c_str() );;
            if ( WEXITSTATUS( ex ) > 0 ) {
                cerr << "nonzero exit: " << *i << ": " << WEXITSTATUS( ex ) << endl;
                exit( WEXITSTATUS( ex ) );
            } else {
                cerr << "zero exit: " << *i << ": " << ex << endl;
            }
        }
        exit( 0 );
    }
    
    int status;
    while ( ::waitpid( child, &status, WNOHANG ) == 0 ) {
        sleep( 3 );
        cerr << "sending data (not final)" << endl;
        printout( dir );
        cout << "R" << endl;;
    }

    printout( dir );
    rmtree( dir );

    cerr << "done: " << WEXITSTATUS( status ) << endl;
    bye( WEXITSTATUS( status ) );
}
