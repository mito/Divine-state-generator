package divine.server;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLServerSocketFactory;

import java.io.*;
import java.lang.*;
import java.util.*;
import divine.common.*;

/**
 * Server part of the project, listens on the specified port and
 * creates new instances of DwiSocket
 * @author xforejt, mornfall
 */

public class DwiServer {

    protected MapNode< User.Passwd > m_users;
    protected TreeMap< DwiSocket, Thread > m_activeSockets;
    private SSLServerSocket m_socket;
    protected String m_dataPath;
    protected int m_port;
    protected static DwiServer s_server;

    public boolean shouldStop() {
        return false; // XXX more useful implementation?
    }

    public static DwiServer the()
        throws Exception
    {
        if ( s_server == null )
            s_server = new DwiServer();
        return s_server;
    }

    public String dataPath( String f ) {
        return m_dataPath + "/" + f;
    }

    public MapNode< User.Passwd > users() { return m_users; }

   // public void addUser( User u ) { m_users.add( u ); }
   // public void detachUser( String name ) { m_users.get( name ).detach(); }

    protected String dataFile( String name )
        throws Exception
    {
        BufferedReader r = new BufferedReader(
                new FileReader( dataPath( name ) ) ) ;
        String l;
        StringBuffer buf = new StringBuffer();
        while ( ( l = r.readLine() ) != null ) {
            buf.append( l + "\n" );
        }
        return buf.toString();
    }

    protected DwiServer()
        throws Exception
    {
    }

    public static void main( String[] args )
        throws Exception
    {
        Logger.setAppname( "dwiserver" );
        String data = System.getProperty( "user.home" ) + "/divine/dwi/store";

        int port = 4848;
        for ( int i = 0; i < args.length; i++ )
        {
            if ( args[ i ].equals( "--testing" ) ) {
                port = 4849;
            }

            if ( args[ i ].equals( "--data" ) ) {
                data = new String( args[ i + 1 ] );
                i = i + 1;
            }
        }

        DwiServer.the().start( port, data );
    }

    public void start( int port, String data )
        throws Exception
    {
        m_port = port;
        m_dataPath = data;

        m_users = new MapNode( null, "users",
                (Map) Wired.from( Wired.mapTemplate( new User.Passwd() ),
                        dataFile( "passwd" ) ) );
        m_activeSockets = new TreeMap();

        try
        {
            /* taskWatcher = new AlgorithmWatcherThread();
               taskWatcher.start(); */

            m_socket = ( SSLServerSocket )
                SSLServerSocketFactory.getDefault().createServerSocket( m_port );
            Logger.log( 3, "socket created" );

            while ( !shouldStop() )
            {
                Logger.log( 4, "listening to client requests" );

                for ( DwiSocket sock : m_activeSockets.keySet() ) { // reap
                    if ( !sock.running() )
                        m_activeSockets.remove( sock );
                }
                
                SSLSocket socket = null;
                try {
                    socket = ( SSLSocket ) m_socket.accept();
                }
                catch ( Exception ex )
                {
                    Logger.log( 1, "server caught exception\n"
                            + ex.getMessage() );
                    Logger.log( ex );
                }
                finally
                {
                    if ( socket != null && !shouldStop() )
                    {
                        DwiSocket s;
                        Thread t =
                            new Thread( s = new DwiSocket( socket ) );
                        t.start();
                    }
                }
            }

            Logger.log( 1, "server shutting down" );
            for ( DwiSocket sock : m_activeSockets.keySet() ) {
                sock.stop();
            }

            int i = 0;
            while ( i < 5 ) {
                Logger.log( 1, "reaping threads, try " + i );
                Thread.sleep( 500 );
                for ( DwiSocket sock : m_activeSockets.keySet() ) {
                    m_activeSockets.get( sock ).interrupt();
                }
                ++i;
            }

            for ( User.Passwd up : users().values() ) {
               Worker.storeUserData( up.user(), dataPath( "user/" + up.user().name() ) );
            }

        }
        catch ( Exception ex )
        {
            Logger.log( 1, "caught exception in DwiServer start method"
                    + ex.getMessage() );
            Logger.log( ex );
        }
    }
}
