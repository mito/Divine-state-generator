// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;

import java.util.*;
import java.io.*;

public class Cluster extends Node
    implements Wired.Interface, Cloneable
{
    protected MapNode< Computer > m_computers;
    protected String m_host;
    protected String m_login;
    protected String m_workDirectory, m_workPath;
    protected boolean m_interactive;
    public enum Type { PBS, Interactive };
    protected Type m_type;

    public boolean interactive() { return m_interactive; }

    public String workDirectory() { return m_workDirectory; }
    public String workPath() { return m_workPath; }

    public class Monitor extends Executable {

        protected boolean m_stop;
        public void stop() { m_stop = true; }

        public void data( String s ) {
            if ( s == null || s.equals( "" ) )
                return;
            try {
                Map m = (Map) Wired.from( Wired.map( "reservations", "" ), s );
                String res = (String) m.get( "reservations" );
                // Logger.log( 4, "reservations: " + res );
                setReservations( res );
            } catch ( Exception e ) {
                Logger.log( e );
            } // stfu, java
        }

        public void run( Cluster c ) {
            /* if ( !interactive() )
               return; */
            m_stop = false;
            List< String > l = new ArrayList();
            l.addAll( m_computers.keySet() );
            Logger.disable();
            while ( !m_stop ) {
                try {
                    execute( c, Wired.to( Wired.map(
			       "command", "showres -n > reservations; qstat >> reservations" 
			       ) ) );
                    Thread.sleep( 10000 ); // ms
                } catch ( Exception e ) {
                    Logger.log( e );
                }
            }
        }
    }

    public class Runner implements Runnable {
        Executable m_executable;
        public Runner( Executable x ) { m_executable = x; }
        public Executable executable() { return m_executable; }
        public void run() {
            m_executable.run( self() );
        }
    }

    public static abstract class Executable
        extends Node
        implements Cloneable
    {
        public enum State { Prepared, Queued, Finished, Aborting, Aborted, Running, NotStarted };
        protected State m_state;
        protected boolean m_abort = false;

        public void abort() {
            m_abort = true;
            m_state = State.Aborted;
            setDirty();
        }

        public State state() { return m_state; }
        public void setState( State s ) { m_state = s; setDirty(); }
        public String stateString() { return stateToString( state() ); }

        public Executable() {
            super( null, "Executable" );
            m_state = State.NotStarted;
        }

        public Executable( Node p, String n ) {
            super( p, n );
            m_state = State.NotStarted;
        }

        public boolean running() { return m_state == State.Running; }
        public boolean mutable() { return m_state == State.NotStarted; }

        public Object clone() { return super.clone(); }

        public String stateToString( State s ) {
            Logger.log( 5, "stateToString: " + s.toString() );
            if ( s == State.Prepared ) return "prepared";
            if ( s == State.Aborting ) return "aborting";
            if ( s == State.Queued ) return "queued";
            if ( s == State.Finished ) return "finished";
            if ( s == State.Running ) return "running";
            if ( s == State.Aborted ) return "aborted";
            if ( s == State.NotStarted ) return "notstarted";
            return "<null>";
        }

        public State stateFromString( String s ) {
            if ( s == null ) return null;
            if ( s.equals( "queued" ) ) return State.Queued;
            if ( s.equals( "prepared" ) ) return State.Prepared;
            if ( s.equals( "finished" ) ) return State.Finished;
            if ( s.equals( "running" ) ) return State.Running;
            if ( s.equals( "aborting" ) ) return State.Aborting;
            if ( s.equals( "aborted" ) ) return State.Aborted;
            if ( s.equals( "notstarted" ) ) return State.NotStarted;
            return null;
        }

        public abstract void run( Cluster c );
        public abstract void data( String s );

        public int execute( Cluster c, String input )
            throws Exception
        {
            m_abort = false;
            m_state = State.Prepared;
            Logger.log( 3, "execute running ssh -o 'BatchMode yes' -A -l "
                    + c.login() + " " + c.host() + " divine.dwi-run" );
            ProcessBuilder pb = new ProcessBuilder( "ssh", "-A", "-l",
                                                    c.login(), c.host(), "PATH=" + c.workPath()
                                                    + " divine.dwi-run" );

            Process p = pb.start();
            BufferedReader out = new BufferedReader(
                    new InputStreamReader( p.getInputStream() ) );
            BufferedReader err = new BufferedReader(
                    new InputStreamReader( p.getErrorStream() ) );
            PrintWriter in = new PrintWriter(
                    new OutputStreamWriter( p.getOutputStream() ) );
            in.println( "work-directory: " + c.workDirectory() );
            in.print( input );
            in.close();

            String ol = "", el = "";
            StringBuffer outb = new StringBuffer(), errb = new StringBuffer();

            boolean outdone = false;
            int retval = -1;
            while ( true ) {

                if ( !err.ready() && !out.ready() )
                    Thread.sleep( 100 );

                while ( err.ready() || outdone ) {
                    el = err.readLine();
                    if ( el == null ) break;
                    errb.append( el + "\n" );
                    Logger.log( 5, "execute collected: (err) " + el );
                }

                while ( !outdone && out.ready() ) {
                    ol = out.readLine();
                    if ( ol.equals( "R" ) ) { // reset
                        Logger.log( 5, "execute collected: (out) " + outb.toString() );
                        data( outb.toString() );
                        if ( state() == State.Finished )
                            break;
                        outb = new StringBuffer();
                    } else if ( !ol.equals( "E" ) )
                        outb.append( ol + "\n" );
                }

                if ( ol.equals( "E" ) )
                    m_state = State.Finished;

                if ( state() == State.Finished )
                    retval = 0;

                if ( state() == State.Finished || m_abort ) {
                    Logger.log( 5, "execute collected: (out) " + outb.toString() );
                    outdone = true;
                    p.destroy();
                    p.waitFor();
                    break;
                }

            }

            Logger.log( 50, "execute finished" );
            try {
                p.getOutputStream().close();
                p.getInputStream().close();
                p.getErrorStream().close();
            } catch ( Exception e ) { Logger.log( e ); }

            if ( !m_abort ) {
                data( outb.toString() );
            }

            if ( retval == -1 ) retval = p.exitValue();
            return retval;
        }
    }

    public Runner monitor() {
        return new Runner( new Monitor() );
    }

    public Runner runner( Executable x ) {
        return new Runner( x );
    }

    public Cluster self() { return this; }
    public String login() { return m_login; }

    public Cluster()
    {
        m_host = "localhost";
        m_name = "cluster";
        m_reservations = "";
    }

    public String host() { return this.m_host; }
    public void setHost( String h ) { this.m_host = h; }

    public MapNode< Computer > computers() { return m_computers; }
    public int computerCount() { return m_computers.size(); }

    public Object clone() { return super.clone(); }

    public List< Computer > leastLoaded( int n ) {
        ArrayList< Computer > r = new ArrayList(), all = new ArrayList();
        all.addAll( computers().values() );
        if ( n >= computerCount() )
            n = computerCount();
        r.addAll( all.subList( 0, n ) );
        for ( Computer c : all.subList( n, all.size() ) ) {
            for ( Computer x : (List< Computer >) r.clone() ) {
                if ( c.load() == -1 )
                    continue;
                if ( c.load() < x.load() || x.load() == -1 ) {
                    r.remove( x );
                    r.add( c );
                    break;
                }
            }
        }
        for ( Computer x : (List< Computer >) r.clone() ) {
            if ( x.load() == -1 )
                r.remove( x );
        }
        return r;
    }

    protected String m_reservations;

    public void setReservations( String r ) {
        m_reservations = r;
        setDirty();
    }

    public String reservations() {
        return m_reservations;
    }

    public Type typeFromString( String t ) {
        if ( t.equals( "pbs" ) ) return Type.PBS;
        if ( t.equals( "interactive" ) ) return Type.Interactive;
        return null;
    }

    public String typeToString( Type t ) {
        if ( t == Type.PBS ) return "pbs";
        if ( t == Type.Interactive ) return "interactive";
        return "<null>";
    }

    public Type type() { return m_type; }
    public String typeString() { return typeToString( m_type ); }

    public Object fromWire( String s )
        throws Exception
    {
        Map< String, Object > m = (Map) Wired.from(
                Wired.map( "host", "", "login", "", "type", "",
                        "work-directory", "", "res", "",
                        "work-path", "$PATH",
                        "computers", Wired.mapTemplate( new Computer() ) ), s );

        m_reservations = (String) m.get( "res" );
        Logger.log( 4, "fromWire: reservations: " + m_reservations );
        m_host = (String) m.get( "host" );
        m_login = (String) m.get( "login" );
        m_workDirectory = (String) m.get( "work-directory" );
        m_workPath = (String) m.get( "work-path" );
        m_type = typeFromString( (String) m.get( "type" ) );

        m_computers = new MapNode< Computer >( this, "computers" );
        if ( m.get( "computers" ) != null )
            m_computers.putAll( (Map) m.get( "computers" ) );

        return this;
    }

    public String toWire() // XXX omits work-directory
        throws Exception
    {
        return Wired.to(
                Wired.map(
                        "login", login(),
                        "type", typeString(),
                        "host", host(),
                        "res", reservations(),
                        "computers", computers() ) );
    }
}
