// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;
import java.io.*;
import java.lang.*;
import java.util.*;
import divine.common.Algorithm;
import divine.common.Cluster;
import divine.common.Model;
import divine.common.Task;
import divine.common.Property;

public class Worker implements Runnable
{
    protected Reader m_input;
    protected Writer m_output;
    protected User m_user;
    protected Protocol m_protocol;
    protected List< Cluster.Monitor > m_monitors;

    public Worker() {
        m_monitors = new ArrayList();
    }
    
    protected String dataPath( String name ) {
        return "./store/" + name;
    }

    // XXX duped from DwiServer
    protected String dataFile( String name )
        throws Exception
    {
        BufferedReader r =
            new BufferedReader( new FileReader( dataPath( name ) ) );
        String l;
        StringBuffer buf = new StringBuffer();
        while ( ( l = r.readLine() ) != null ) {
            buf.append( l + "\n" );
        }
        return buf.toString();
    }

    public Cluster utilCluster() { // XXX, make configurable
        String k = (String) user().clusters().keySet().toArray()[ 0 ];
        Cluster c = user().clusters().get( k );
        Logger.log( 5, c.toString() );
        return c;
    }

    public User user() { return m_user; }

    public void setInput( Reader r ) { m_input = r; }
    public void setOutput( Writer w ) { m_output = w; }

    public MapNode< Algorithm > loadAlgorithms() throws Exception {
        return new MapNode( null, "algorithms",
                (Map) Wired.from( Wired.mapTemplate( new Algorithm() ),
                        dataFile( "algorithms" ) ) );
    }

    public MapNode< Cluster > loadClusters() throws Exception {
        return new MapNode( null, "clusters",
                (Map) Wired.from( Wired.mapTemplate( new Cluster() ),
                        dataFile( "clusters" ) ) );
    }

    public void initProtocol()
        throws Exception
    {
        // initialize Protocol handlers
        m_protocol = new Protocol();
        m_protocol.setHandler( "login", this, "handleLogin" );
        m_protocol.setHandler( "logout", this, "handleLogout" );

        m_protocol.setHandler( "store-model", this, "handleStoreModel" );
        m_protocol.setHandler( "delete-model", this, "handleDeleteModel" );
        m_protocol.setHandler( "check-syntax", this, "handleCheckSyntax" );

        m_protocol.setHandler( "query-user-data", this, "handleQueryUserData" );
        m_protocol.setHandler( "query-changes", this, "handleQueryChanges" );
        m_protocol.setHandler( "store-changes", this, "handleStoreChanges" );

        m_protocol.setHandler( "execute-job", this, "handleExecuteJob" );
        m_protocol.setHandler( "abort-job", this, "handleAbortJob" );
    }

    public void closeIO() {}

    public static void storeUserData( User u, String p ) throws Exception
    {
        // back up current file
        (new File( p )).renameTo( new File( p + ".bak" ) );
        FileWriter w = new FileWriter( p, false );
        w.write( u.userData() );
        w.close();
    }

    public void storeUserData() throws Exception {
        storeUserData( user(), dataPath( "user/" + user().name() ) );
    }

    public void deactivateUser() throws Exception {
        user().setActiveInternal( false );
        for ( Cluster.Monitor m : m_monitors ) {
            m.stop();
        }
    }

    public void loginLocal() throws Exception {
        m_user = loginUser(
                new Request( "dummy-login", Wired.map(), Wired.map() ), "#local", "" );
    }

    protected void activateUser( User u ) throws Exception {
        u.setClusters( loadClusters() );
        u.setAlgorithms( loadAlgorithms() );
        u.setActiveInternal( true );
        for ( Cluster c : u.clusters().values() )
        {
            Logger.log( 5, "running cluster monitor for " + c.name() );
            Cluster.Runner m = c.monitor();
            m_monitors.add( (Cluster.Monitor) m.executable() );
            Thread t = new Thread( m );
            t.setDaemon( true );
            t.start();
        }

        try {
            u.loadUserData( dataFile( "user/" + u.name() ) );
        } catch ( FileNotFoundException e ) {} // ignore
    }

    protected User loginUser( Request r, String u, String p )
        throws Exception
    {
        User ret = new User();
        ret.setName( u );
        activateUser( ret );
        return ret;
    }

    public void handleLogin( Request r, Map< String, Object > a )
        throws Exception
    {
        String user = a.get( "user" ).toString();
        String pass = a.get( "pass" ).toString();
        Logger.log( 5, "version: " + a.get( "version" ).toString() );
        Integer version = Integer.parseInt( a.get( "version" ).toString() );
        if ( version != Protocol.version() ) {
           r.respond( Wired.map( "error", "requested protocol version not supported: "
                       + "server is " + Protocol.version().toString()
                       + "and you asked for " + version.toString() ) );
        }
        Logger.log( 1, "login requested for " + user );
        if ( user() != null ) {
            r.respond( Wired.map( "error",
                            "INTERNAL ERROR: user already logged in for this thread" ) );
        } else {
            m_user = loginUser( r, user, pass );
        }
    }

    public void handleLogout( Request r, Map< String, Object > a )
        throws Exception
    {
        // XXX save in case the loop fails to save user data?
        storeUserData(); 
        r.respond( Wired.map( "status", "success" ) );
        stop();
    }

    public void checkLogged() throws Exception {
        if ( user() == null )
            throw new Exception( "user not logged in" );
    }

    public void handleQueryUserData( Request r, Map< String, Object > m )
        throws Exception
    {
        Logger.disable();
        checkLogged();
        Logger.log( 6, "sending user data" );
        r.respond( Wired.map( "data", user() ) );
        user().clearDirtyRec();
        Logger.enable();
    }

    public void handleQueryChanges( Request r, Map< String, Object > m )
        throws Exception
    {
        Logger.disable();
        checkLogged();
        Logger.log( 6, "sending user data" );
        r.respond( Wired.map( "changes", user().dirtyMap() ) );
        user().clearDirtyRec();
        Logger.enable();
    }

    public void handleStoreChanges( Request r, Map< String, Object > m )
        throws Exception
    {
        Map< String, String > changes = (Map) m.get( "changes" );
        Logger.log( 4, "store changes: " + Wired.to( changes ) );
        user().mergeNodeMap( changes );
        user().prune(); // we generally don't delete anything, so we
                        // can as well prune here
        storeUserData();
        r.respond( Wired.map() );
    }

    public void handleCheckSyntax( Request r, Map< String, Object > map )
        throws Exception
    {
        String mod = (String) map.get( "model" );
        String prop = (String) map.get( "property" );
        Model m = user().models().get( mod );
        Property p = null;
        if ( prop != null )
            p = m.properties().get( prop );
        if ( m == null )
            r.respond( Wired.map( "status", "unknown model" ) );
        else {
            // check the model
            r.respond( m.syntaxCheck( utilCluster(), p ) );
            // r.respond( Wired.map( "ok", "syntax ok" ) );
        }
    }

    public void handleExecuteJob( Request r, Map< String, Object > m )
        throws Exception
    {
        checkLogged();
        Logger.log( 6, "executing job");
        Job j = (Job) user().find( (Node.Path) m.get( "path" ) );
        if ( j == null )
            r.respond( Wired.map( "status", "no such job on server" ) );
        else {
            j.execute();
            r.respond( Wired.map( "status", "success" ) );
        }
    }

    public void handleAbortJob( Request r, Map< String, Object > m )
        throws Exception
    {
        checkLogged();
        Logger.log( 6, "aborting job");
        Job j = (Job) user().find( (Node.Path) m.get( "path" ) );
        j.abort();
        r.respond( Wired.map( "status", "success" ) );
    }

    private boolean m_stop = false;

    public void stop() { m_stop = true; }
    public boolean running() { return !m_stop; }

    public void run()
    {
        while ( !m_stop )
        {
            try {
                m_protocol.handle( m_input, m_output );
            } catch ( NullPointerException e ) {
                Logger.log( e );
            } catch ( IOException e ) {
                Logger.log( 1, "input/output error on socket, dying" );
                stop(); // can't continue on IO errors
            } catch ( Exception e ) {
                Logger.log( e );
                // XXX this is a massive hack
                if ( e.getMessage() != null
                        && e.getMessage().equals( "EOF while reading from socket" ) )
                    stop();
            }
        }

        closeIO();

        try {
            deactivateUser();
            Logger.log( 3, "Worker shutting down" );
        } catch ( Exception e ) {}

    }

}
