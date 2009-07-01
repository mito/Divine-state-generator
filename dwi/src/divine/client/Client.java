// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;
import java.util.*;
import java.util.regex.*;
import java.io.*;
import javax.net.ssl.SSLSocket;

import divine.common.*;

public class Client {

    protected MainWindow m_mainWindow;
    public MainWindow mainWindow() { return m_mainWindow; }
    protected void setMainWindow( MainWindow w ) { m_mainWindow = w; }
    static protected int m_port;

    class LocalWorker extends Worker {
        protected String dataPath( String name ) {
            return dataDir() + "/" + name;
        }
    }

    private javax.swing.Timer m_timer;

    protected User m_user;
    protected Protocol m_protocol;
    protected LocalWorker m_local = null;
    protected Thread m_localThread;
    public User user() { return m_user; }
    public MapNode< Cluster > clusters() { return user().clusters(); }
    public MapNode< Algorithm > algorithms() { return user().algorithms(); }
    public MapNode< Model > models() { return user().models(); }

    /** Creates new form MainWindow */
    public Client() {

        m_protocol = new Protocol();
        m_imageDir = "images";

        m_timer = new javax.swing.Timer( 5000,
                new java.awt.event.ActionListener()
                {
                    public void actionPerformed( java.awt.event.ActionEvent evt )
                    {
                        queryChanges();
                    }
                }
            );
        m_timer.setInitialDelay( 1 );
        m_timer.setCoalesce( true );
    }

    protected String m_imageDir = "./images";
    protected String m_dataDir = "./store";

    public String imageDir() { return m_imageDir; }
    public String dataDir() { return m_dataDir; }

    public void setImageDir( String s ) { m_imageDir = s; }
    public void setDataDir( String s ) { m_dataDir = s; }

    protected boolean logout() {
        m_timer.stop();
        if ( user() == null )
            return true;
        storeChanges();
        user().models().clear();

        boolean err = false;
        Map< String, Object > m = new TreeMap(); // stupid
        try {
            m = request( "logout", null );
        } catch (Exception e) {
            Logger.log( e );
            err = true;
        }

        if ( err || !m.get( "status" ).equals( "success" ) ) {
            /* messageBox( "Error logging out. Please restart the application."
                    + ( err ? "" : " Server said: " + (String) m.get( "error" ) ),
                    "Logout failed",
                    javax.swing.JOptionPane.ERROR_MESSAGE ); */
            return false;
        }
        return true;
    }

    public String readFile( File f ) {
        try {
            java.io.FileReader fr = new java.io.FileReader( f.getAbsolutePath() );
            char[] buffer = new char[ 1024 ];
            int length = 0;
            StringBuilder sb = new StringBuilder();

            while ( ( length = fr.read( buffer ) ) > 0 )
            {
                sb.append( buffer, 0, length );
            }
            return sb.toString();
        } catch ( Exception e ) {
            Logger.log( e );
        }
        return null;
    }

    public Map< String, Object > request( String r, Map< String, ? extends Object > a )
    // throws Exception
    {
        Map< String, Object > ret = new TreeMap();
        try {
            if (a == null)
                a = new TreeMap< String, Object >();
            Logger.log( 1, "MainWindow.request: " + r );
            ret = m_protocol.find( r ).run( output, input, a );
        } catch ( Exception e ) {
            Logger.log( e );
            /* messageBox( "Request failed with exception: "
                        + e.getClass().toString() +
                        (e.getMessage() == null ?
                             "" : " (" + e.getMessage() + ")" )
                        + " Please contact site administrator.",
                        "Request Failed",
                        javax.swing.JOptionPane.ERROR_MESSAGE ); */
        }
        return ret;
    }

    public void queryChanges() {
        try {
            Map< Node.Path, Job.State > current = new HashMap();
            for ( Task t : user().tasks().values() )
                for ( Job j : t.jobs().values() )
                    current.put( j.path(), j.state() );
            // Logger.log( 2, "queryChanges running" );
            MainWindow.TreeState ts = mainWindow().treeState();

            Map< String, String > changed = 
                (Map) request( "query-changes", null ).get( "changes" );

            if ( changed.size() > 0 )
                Logger.log( 5, "changes: " + changed );

            mainWindow().syncTree();
            user().mergeNodeMap( changed );
            // user().prune();

            for ( Task t : user().tasks().values() )
                for ( Job j : t.jobs().values() )
                    if ( current.get( j.path() ) != j.state() ) {
                        Logger.log( 1, "state of " + j.name() + " changed to " + j.stateString() );
                        mainWindow().notifyJobChange( j );
                    }

            mainWindow().treeRestore( ts );
            mainWindow().updatePanel();
            mainWindow().updateToolbar();
            
//            System.out.println("HELE_BEGIN: " + changed + "HELE_END");
        } catch ( Exception e ) {
            Logger.log( e );
        }
    }

    public Model importModel( File file )
    {
        String content = readFile( file );
        if ( content == null ) return null;
        String filename = file.getName();

        if ( filename.endsWith( "mdve" ) )
        {
            //if user selected "mdve" file, let him change values of variables
            content = "mdve reading to be fixed";
            // MdveToDveDialog dlg = new MdveToDveDialog( cl, true, content );
            // dlg.setVisible( true );
            // if ( dlg.getReturnValue() == 1 )
            // {
            // content = dlg.getConvertedModel();
            // }
        }

        Model m = new Model();
        if ( filename.endsWith( "pml" ) )
            m.setType( Model.Type.PML );
        else if ( filename.endsWith( "dve" ) )
            m.setType( Model.Type.DVE );

        m.setText( content );
        m.setNameInternal( filename.substring( 0, filename.lastIndexOf( '.' ) ) );
        models().addUnique( m );
        return m;
    }

    public Property importProperty( Model m, File file )
    {
        String content = readFile( file );
        if ( content == null ) return null;
        String filename = file.getName();

        Property p = new Property();
        p.setType( Property.Type.LTL );
        p.setText( content );
        p.setNameInternal( filename.substring( 0, filename.lastIndexOf( '.' ) ) );
        m.addProperty( p );
        return p;
    }

    public Model createModel( Model.Type t ) {
        Model m = new Model();
        models().addUnique( m );
        m.setType( t );
        storeChanges();
        return m;
    }

    public void storeChanges() {
        try {
            Logger.log( 2, "storeChanges running" );
            String changed = Wired.to( user().dirtyMap() );
            user().clearDirtyRec();
            user().prune();
            mainWindow().syncTree();
            Logger.log( 5, "changes: " + changed );
            request( "store-changes", Wired.map( "changes", changed ) );
        } catch ( Exception e ) {
            Logger.log( e );
        }
    }

    /* protected boolean ensurePropertyValid( Property p ) {
        if ( !p.typeValid() ) {
            JOptionPane.showMessageDialog( this,
                    "Combination of selected model type and property "
                    + "type is not valid", "Property type mismatch",
                    JOptionPane.ERROR_MESSAGE );
            return false; 
        }
        
        if ( checkSyntax( null, p ) != null ) {
            JOptionPane.showMessageDialog( this,
                    "Property or model is not valid",
                    "Invalid property/model", JOptionPane.ERROR_MESSAGE );
            return false;
        }
        
    *//* if ( !task().property().model().typeValid() ) {
           JOptionPane.showMessageDialog( this,
           "The model type does not agree with content",
           "Model type mismatch", JOptionPane.ERROR_MESSAGE );
           return;
           } *//*
        
        return true;
        } */

    protected void executeJob( Job j ) {
        try {
            j.setState( Job.State.Prepared );
            mainWindow().updateToolbar();
            mainWindow().updatePanel();
            storeChanges();
            // if ( !ensurePropertyValid( t.property() ) ) return;
            request( "execute-job", Wired.map( "path", j.path() ) );
        } catch ( Exception e ) {
            Logger.log( e );
        }
        mainWindow().updatePanel();
    }

    protected void abortJob( Job j ) {
        try {
            j.setState( Job.State.Aborting );
            request( "abort-job", Wired.map( "path", j.path() ) );
        } catch ( Exception e ) {
            Logger.log( e );
        }
        mainWindow().updatePanel();
        mainWindow().updateToolbar();
    }

    protected void deleteObject( Node o ) {
        o.setDeleted();
        user().deleteOrphanedTasks();
        mainWindow().selectNode( mainWindow().visibleParent( o ) );
        mainWindow().syncTree();
        mainWindow().updatePanel();
    }

    public Node renameObject( Node o, String n ) {
        if ( n == null || n.equals( "" ) ) return null;
        Node cloned = (Node) o.clone();
        o.setDeleted();
        cloned.rename( n );
        mainWindow().syncTree();
        mainWindow().selectNode( cloned );
        storeChanges();
        return cloned;
    }

    public String checkSyntax( Model m, Property p ) {
        m = m == null ? p.model() : m;
        storeModel( m );
        Map< String, Object > resp = request(
                "check-syntax", Wired.map( "model", m.name(),
                        "property", p != null ? p.name() : "" ) );
        return (String) resp.get( "status" );
    }

    public void storeModel( Node o )
    {
        while ( o != null && ! ( o instanceof Model ) )
            o = o.parent();
        if ( o == null )
            return; // throw?

        Model m = (Model) o;

        // Logger.log( 5, "storing model: " + Wired.to( m ) );
        Map< String, Object > resp = request( "store-model",
                                              Wired.map( "name", m.name(),
                                                         "model", m ) );

        if ( resp.get( "status" ).equals( "success" ) ) {
            m.clearDirty();
            Logger.log( 4, "model stored ok" );
        } else {
            Logger.log( 4, "error storing model" );
            /* messageBox( "Model not saved. "
                    + "Please save the model locally and contact "
                    + "site administrator. Server said: "
                    + resp.get( "error" ).toString(), "Error saving model",
                    javax.swing.JOptionPane.ERROR_MESSAGE ); */
        }
    }

    public void setLocal( boolean l ) {
        if ( l ) {
            m_local = new LocalWorker();
            // m_actLogout.setEnabled( false );
        }
    }

    // LOGIN
    public void login()
    {
        if ( m_local != null ) {
            try {
                output = new PipedWriter();
                input = new PipedReader();
                PipedReader workerin = new PipedReader( (PipedWriter) output );
                PipedWriter workerout = new PipedWriter( (PipedReader) input );
                m_local.setInput( workerin );
                m_local.setOutput( workerout );
                m_local.initProtocol();
                mainWindow().setLocalMode( true );
                m_user = new User();
                m_user.setNameInternal( "#local" ); // XXX hack
                // m_local.loadAlgorithms();
                // m_local.loadClusters();
                m_local.loginLocal(); // login local user
                m_localThread = new Thread( m_local );
                m_localThread.start();
            } catch ( Exception e ) { Logger.log( e ); }
        } else {

            LoginDialog frm = new LoginDialog( mainWindow(), m_port );
            frm.setVisible( true );

            if ( frm.user() == null )
            {
                System.exit( 0 );
            }
            m_user = frm.user();
            mainWindow().setTitle( "DiVinE: user " + user().name()
                                   + " at " + frm.getServername() );
        }

        try {
            queryUserData();
            m_timer.start();
        } catch (Exception e) {
            Logger.log( e );
        }

        m_timer.start();
        mainWindow().updatePanel();
        mainWindow().updateToolbar();

    }

    private void queryUserData()
        throws Exception
    {
        Logger.log( 4, "queryUserData()" );
        Map< String, Object > map = request( "query-user-data", null );
        String name = user().name();
        if ( map.get( "data" ) != null )
            m_user = (User) map.get( "data" );
        m_user.setNameInternal( name );
        user().clearDirtyRec();
        mainWindow().syncTree();
    }

    // MAIN function
    public static void main( String args[] )
    {
        m_port = 4848;
        Logger.setAppname( "dwiclient" );
        Logger.setMaxPriority( 0 );
        boolean errorDuringParsing = false;

        //if user requests help, just show it and return
        boolean help = false;
        for ( int i = 0; i < args.length; i++ )
        {
            if ( args[ i ].equals( "--help" ) )
            {
                help = true;
                break;
            }
        }

        if ( help )
        {
            System.err.println( "Usage:\n" );
            System.err.println( "--help             Show this help" );
            System.err.println( "--imagedir <path>  Set image and icons directory [images/]" );
            System.err.println( "--datadir <path>   Set path to data directory [store/]" );
            return ;
        }

        final Client c = new Client();
        for ( int i = 0; i < args.length; i++ )
        {
            if ( args[ i ].equals( "--debug" ) )
                Logger.setMaxPriority( 25 );

            if ( args[ i ].equals( "--testing" ) )
                m_port = 4849;

            if ( args[ i ].equals( "--local" ) )
                c.setLocal( true );

            if ( args[ i ].equals( "--datadir" ) ) {
                c.setDataDir( args[ ++i ] );
            }

            if ( args[ i ].equals( "--imagedir" ) ) {
                c.setImageDir( args[ ++i ] );
            }
        }
        final MainWindow win = new MainWindow( c );
        c.setMainWindow( win );

        if ( errorDuringParsing )
        {
            System.err.println( "An error occured while parsing program arguments."
                    + " Correct the error or use \"--help\" for help." );
            return ;
        }

        //if parsed correctly, run the client
        java.awt.EventQueue.invokeLater( new Runnable() {
                public void run() {
                    win.setVisible( true );
                    c.login();
                }
            }
            );
    }

    // SOCKET handling
    private SSLSocket socket;
    public void setSocket( SSLSocket socket ) throws Exception
    {
        this.socket = socket;
        this.output = new OutputStreamWriter( this.socket.getOutputStream() );
        this.input = new InputStreamReader( this.socket.getInputStream() );
    }

    public Reader input;
    public Writer output;

}
