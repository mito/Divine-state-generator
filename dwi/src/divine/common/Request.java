// generic Request implementation

package divine.common;

import divine.common.Wired;
import java.util.*;
import java.util.regex.*;
import java.io.*;
import java.lang.*;
import java.lang.reflect.*;

public class Request {


    protected Map< String, Object > m_arguments;
    protected Map< String, Object > m_response;

    protected String m_id;
    protected Object m_handler;
    protected String m_handlerMethod;

    protected BufferedReader m_in;
    protected PrintWriter m_out;
    
    public Map< String, Object > arguments()
    {
        return m_arguments;
    }

    public Request( String id, Map< String, Object > args, Map< String, Object > resp )
    {
        m_arguments = args;
        m_response = resp;
        m_id = id;
    }

    public void setHandler( Object h, String m )
    {
        m_handler = h;
        m_handlerMethod = m;
    }

    public Map< String, Object > run(
            Writer out,
            Reader in,
            Map< String, ? extends Object > al ) throws Exception
    {
        checkTypes( al, m_arguments );
        write( out, "R", al ); // request
        Map< String, Object > v = read( in, "D", m_response ); // done
        checkTypes( v, m_response );
        return v;
    }

    public void doHandle( Reader _in, Writer _out ) throws Exception
    {
        BufferedReader in = new BufferedReader( _in );
        PrintWriter out = new PrintWriter( _out );
        m_in = in; m_out = out;
        Map< String, Object > v;
        try {
            // Logger.log( 4, "doHandle reading..." );
            v = read( m_in, "R", m_arguments );
            checkTypes( v, m_arguments );
        } catch ( Exception e ) {
            Logger.log( 4, "exception raised while reading from socket" );
            Logger.log( e );
            String error = e.getClass().getName()
                + (e.getMessage() == null ? "" : e.getMessage() );
            respond( Wired.map( "status",
                            "exception raised while reading from socket: " + error ) );
            throw e;
        }
        try {
            Method meth = m_handler.getClass()
                .getMethod( m_handlerMethod, getClass(), Map.class );
            Logger.log( 4, "doHandle running handler " + m_handlerMethod );
            meth.invoke( m_handler, this, v );
        } catch ( Exception e ) {
            Logger.log( 4, "exception raised in handler method " + m_handlerMethod );
            Logger.log( e );
            String error = e.getClass().getName()
                + (e.getMessage() == null ? "" : e.getMessage() );
            respond( Wired.map( "status", error ) );
            throw e;
        }
    }

    public void respond( Map< String, ? extends Object > al ) throws Exception
    {
        checkTypes( al, m_response );
        write( m_out, "D", al );
    }

    protected static void checkTypes( Map< String, ? extends Object > real,
                                      Map< String, ? extends Object > formal )
        throws Exception
    {
        // XXX reconsider this
        if (real == null)
            throw new Exception( "internal error obtaining parameters" );
        /* if (real.size() != formal.size())
           throw new Exception( "argument count does not match" ); */
        /* for ( String key : real.keySet() ) {
            if ( formal.get( key ) == null )
                throw new Exception( "unknown parameter "
                        + key + "; known parameters: " + formal.keySet().toString() );
            if ( ( formal instanceof Map && ! ( real instanceof Map ) )
                    || ( formal instanceof List && ! ( real instanceof List ) ) )
                throw new Exception ( "real argument type "
                                      + real.get( key ).getClass().toString()
                                      + " does not match expected "
                                      + formal.get( key ).getClass().toString() );
                                      } */
    }

    public void write( Writer _out, String what,
                       Map< String, ? extends Object > al )
        throws Exception
    {
        PrintWriter out = new PrintWriter( _out );
        StringBuffer b = new StringBuffer();
        b.append( what + ": " + m_id + "\n" );
        b.append( Wired.prefixLines( "A: ", Wired.to( al ) ) );
        b.append( "E" );
        out.println( b.toString() );
        out.flush();
        // Logger.log( 6, "wrote: " + b.toString() );
    }

    public Map< String, Object > read( Reader _in, String what,
                             Map< String, Object > template ) throws Exception
    {
        BufferedReader in = new BufferedReader( _in );
        StringBuffer args = new StringBuffer();
        String l = in.readLine();
        if ( l == null )
            throw new Exception( "unexpected end of message at start" );
        if ( !l.equals( what + ": " + m_id ) )
            throw new Exception( "error parsing message ('" + l + "' != '"
                                 + what + ": " + m_id + "')" );
        while ( true ) {
            l = in.readLine();
            if (l == null)
                throw new Exception( "unexpected end of message" );
            if (l.charAt( 0 ) == 'E')
                break;
            else if (l.charAt( 0 ) == 'A')
                args.append( l + "\n" );
            else
                throw new Exception( "unexpected message line: " + l );
        }

        // Logger.log( 6, "read: " + args.toString() );

        Map< String, String > argm = Wired.unprefix( "A: ", args.toString() );
        String argstr = argm.get( "A: " );
        Map ret = (Map) Wired.from( template, argstr == null ? "" : argstr );
        // Logger.log( 6, "done parsing" );
        return ret;
    }
};
