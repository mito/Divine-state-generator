// -*- mode: java; c-basic-offset: 4 -*- 
package divine.server;
import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;
import divine.common.*;

public class DwiSocket extends Worker
{
    protected Socket m_socket;
    
    public Cluster utilCluster() { // XXX, make configurable
        return user().clusters().get( "DiVinE" );
    }

    public DwiSocket( Socket s )
        throws Exception
    {
        BufferedReader input;
        PrintWriter output;

        m_socket = s;
        input = new BufferedReader( new InputStreamReader(
                m_socket.getInputStream() ), 50000 );
        output = new PrintWriter( new OutputStreamWriter(
                m_socket.getOutputStream() ), true );

        setInput( input );
        setOutput( output );

        // setAlgorithms( DwiServer.the().algorithms() );
        // setClusters( DwiServer.the().clusters() );
        initProtocol();
    }

    public String dataPath( String n ) {
        try {
            Logger.log( 4, "DwiSocket data path for " + n + ": " + DwiServer.the().dataPath( n ) );
            return DwiServer.the().dataPath( n );
        } catch ( Exception e ) {
            Logger.log( e );
            return "./store/" + n;
        }
    }

    public void storeUserData() throws Exception {
        if ( user().active() )
            super.storeUserData();
        else
            throw new Exception( "Cannot store data for inactive user" );
    }

    // needed by DwiServer as well
    public void deactivateUser() throws Exception {
        if ( user().active() ) {
            storeUserData();
            user().setActiveInternal( false );
        }
    }

    public User loginUser( Request r, String user, String pass )
        throws Exception
    {
        User u = DwiServer.the().users().get( user ).user();
        m_user = u;
        Logger.log( 4, "handleLogin got " + u.name() );
        if ( u != null && u.passwordOk( pass ) )
        {
            if ( u.active() ) {
                r.respond( Wired.map( "status",
                                "user already active (probably logged"
                                + " in from another computer?" ) );
                return null;
            } else {
                if ( !u.active() )
                    activateUser( u );
                Logger.log( 4, "login succesful: " + user );
                r.respond( Wired.map( "status", "success" ) );
                return u;
            }
        }
        else
        {
            Logger.log( 4, "login error: " + user );
            r.respond( Wired.map( "status", "bad login or password" ) );
            return null;
        }
    }

    public void closeIO() {
        try {
            m_socket.close();
        } catch (Exception e) {
            Logger.log( e );
        }
    }

}
