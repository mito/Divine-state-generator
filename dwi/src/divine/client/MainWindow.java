// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;
import java.util.*;
import java.util.regex.*;
import java.io.*;
import javax.net.ssl.SSLSocket;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.Container;
import java.awt.Font;
import java.awt.Component;
import java.awt.event.*;
import javax.swing.tree.*;

import divine.client.others.ExtensionFilter; // XXX

import divine.common.*;

public class MainWindow extends javax.swing.JFrame {

    protected MainWindow self() { return this; }

    public static class NodePanel extends javax.swing.JPanel {
        public Node node() { return null; };
        public boolean update( Node n ) { return false; }
        public static void setEnabledRec( Component c, boolean e ) {
            if ( c == null ) // ignore null components
                return;
            if ( c instanceof Container )
                for ( Component x : ( (Container) c ).getComponents() )
                    setEnabledRec( x, e );
            if ( c instanceof JScrollBar )
                return; // ignore scrollbars
            c.setEnabled( e );
        }
    }

    protected NodePanel m_currentPanel;
    protected Node m_displayedNode;
    protected Mode m_mode;
    protected Client m_client;

    public Client client() { return m_client; }
    public User user() { return client().user(); }
    public MapNode< Cluster > clusters() { return user().clusters(); }
    public MapNode< Algorithm > algorithms() { return user().algorithms(); }
    public MapNode< Model > models() { return user().models(); }
    public MapNode< Task > tasks() { return user().tasks(); }
    public MapNode< Profile > profiles() { return user().profiles(); }

    public enum Mode { All, Clusters, Models, Tasks };

    /** Creates new form MainWindow */
    public MainWindow( Client c ) {
        m_client = c;

        initActions();
        initComponents();

        m_treenodes = new HashMap();
        tree.getSelectionModel().setSelectionMode(
                TreeSelectionModel.SINGLE_TREE_SELECTION );
        tree.setCellRenderer( new CellRenderer( this ) );

        // splitTop.resetToPreferredSizes();

        setFontRec( this, new Font( "Dialog", Font.PLAIN, 12 ) );

        tree.setModel( new DefaultTreeModel( treenode( currentRoot() ) ) );
        setMode( Mode.All );
        pack();
    }

    public static void setFontRec( Component c, Font f ) {
        if ( c == null ) // ignore null components
            return;
        c.setFont( f );
        if ( c instanceof Container )
            for ( Component x : ( (Container) c ).getComponents() )
                setFontRec( x, f );
    }

    private static String searchDir;
    /**
     * Returns the name of the directory whose content should be shown
     * if user is about to select a file from his hard drive.
     */
    public static String getSearchDir()
    {
        return searchDir;
    }

    /**
     * Sets the name of the directory whose content should be shown if
     * user is about to select a file from his hard drive.
     */
    public static void setSearchDir( String sd )
    {
        searchDir = sd;
    }

    public void messageBox( String text, String title, int t ) {
        JOptionPane.showMessageDialog( this, text, title, t );
    }

    public File chooseFile( String... ext ) {
        JFileChooser chooser = new JFileChooser();
        ExtensionFilter filter = new ExtensionFilter( ext );
        if ( MainWindow.getSearchDir() != null )
            chooser.setCurrentDirectory( new File( getSearchDir() ) );
        chooser.setFileFilter( filter );
        int returnVal = chooser.showOpenDialog( null );
        if ( returnVal == JFileChooser.APPROVE_OPTION ) {
            MainWindow.setSearchDir( chooser.getCurrentDirectory().getAbsolutePath() );
            return chooser.getSelectedFile();
        }
        return null;
    }

    public String readFile( JFileChooser chooser, String... ext ) // XXX
    {
        try {
            return client().readFile( chooseFile( ext ) );
        } catch ( Exception e ) {
            Logger.log( e );
        }
        return null;
    }

    static class TaskStateInfo {
        protected Task m_task;
        protected String m_state;
        public Task task() { return m_task; }
        public String toString() { return
                task().name()
                + " for " + task().property().model().name()
                + " changed state to: " + m_state; }
        public TaskStateInfo( Task t ) {
            m_task = t;
            m_state = "foobar"; // t.stateString();
        }
    }

    public static class TreeState {
        public Set< Node.Path > expanded = new HashSet();
        public Node.Path selected = new Node.Path();
        public boolean clustersExpanded = false;
    }

    public TreeState treeState() {
        Logger.disable();
        Logger.log( 4, "treeState() running" );
        TreeState ret = new TreeState();

        if ( user() == null )
            return ret;

        if ( selectedNode() != null )
            ret.selected = selectedNode().path();

        for ( Node o : user().childrenList() ) {
            if ( treenode( o ) == null ) {
                Logger.log( 4, "ignoring null treenode for: " + o.name()  );
                continue;
            }
            Logger.log( 4, "testing node expanded: " + o.name() );
            if ( treenodeExpanded( treenode( o ) ) ) {
                Logger.log( 4, "node expanded: " + o.name() );
                ret.expanded.add( o.path() );
            }
        }

        Logger.enable();
        return ret;
    }

    public void treeRestore( TreeState s ) {
        Logger.disable();

        if ( user() == null ) return;

        for ( Node.Path p : s.expanded ) {
            try {
                Logger.log( 4, "expanding: " + p.toWire() );
                Logger.log( 4, "expanding (the object): " + user().find( p ).toString() );
            } catch ( Exception e ) {}
            expandTreenode( treenode( user().find( p ) ) );
        }

        if ( s.selected != null ) {
            try {
                Logger.log( 4, "selecting: " + s.selected.toWire() );
            } catch ( Exception e ) {}
            selectNode( user().find( s.selected ) );
        }
        Logger.enable();
    }

    public Node currentRoot() {
        if ( m_mode == Mode.Clusters ) return clusters();
        if ( m_mode == Mode.Models ) return models();
        if ( m_mode == Mode.Tasks ) return tasks();
        return user();
    }

    public Node visibleParent( Node o ) {
        if ( o instanceof Cluster || o instanceof Model
                || o instanceof Task || o instanceof Profile
                || o instanceof Algorithm )
            return o.parent();
        if ( o.parent() instanceof MapNode )
            return o.parent().parent();
        return o.parent();
    }

    public void syncNode( Node o ) {
        try { 
            Logger.log( 2, "syncing node: " + o.path().toWire() );
        } catch (Exception e) {}
        if ( o instanceof Property && ((Property) o).type() != Property.Type.LTL )
            return;
        if ( o instanceof Algorithm || o == user().algorithms() )
            return;
        if ( o instanceof Profile || o == user().profiles() )
            return;
        else if ( o.parent() instanceof MapNode )
            createTreenode( o, visibleParent( o ) );
        else if ( o instanceof MapNode )
            if ( ! (o.parent() instanceof User) )
                return;
        else
            createTreenode( o, visibleParent( o ) );
    }

    public void syncTree() {

        if ( user() == null ) return;
        if ( tree.isEditing() ) return;

        if ( treenode( user() ) == null ) {
            m_treenodes.put( user(), new DefaultMutableTreeNode( user() ) );
        }

        Logger.disable();
        m_syncingTree = true;

        TreeState state = treeState();
        Logger.log( 3, "expanded: " + state.expanded.toString() );

        // we need to do it like this, since removeAllChildren does not generate
        // the events that cause tree to be updated... (wtf)
        try {
            DefaultMutableTreeNode m, n =
                (DefaultMutableTreeNode) treenode( user() ).getFirstChild();

            while ( n != null ) {
                m = n.getNextSibling();
                removeNode( node( n ) );
                n = m;
            }
        } catch ( NoSuchElementException e ) {}

        // user().activateModels();

        for ( Node o : user().childrenList() )
            syncNode( o );

        tree.setModel( new DefaultTreeModel( treenode( currentRoot() ) ) );

        treeRestore( state );
        m_syncingTree = false;
        Logger.enable();
    }

    public Node selectedNode() {
        if ( tree == null ) return null;
        return node( tree.getLastSelectedPathComponent() );
    }

    public Node currentNode() {
        Node n = selectedNode();
        if ( n == null )
            n = currentRoot();
        while ( n.deleted() )
            n = n.parent();
        return n;
    }

    public Node node( Object n ) {
        if ( n == null ) return null;
        return ( Node )
            ( ( DefaultMutableTreeNode ) n ).getUserObject();
    }

    public int confirmDialog( String text, String cap ) {
        return JOptionPane.showConfirmDialog( this, text, cap,
                JOptionPane.YES_NO_OPTION, javax.swing.JOptionPane.WARNING_MESSAGE );
    }

    protected void deleteObjectConfirm( Node o ) {
        int answer = 1;
        if ( o instanceof Model )
        {
            answer = confirmDialog( "Really do you want to delete this model?\n"
                    + "Deleting model is permanent action and cannot be undone",
                    "Confirm deletion" );
        } else {
            answer = confirmDialog( "Do you really want to delete this item?\n"
                    + "Deletion will take effect when you save parent model",
                    "Confirm deletion" );
        }
        if ( answer == 0 )
            client().deleteObject( o );
        // syncTree();
    }

    public List< Action > actionList( Node n ) {
        List< Action > r = new Vector();

        m_actDelete.setEnabled( true );
        m_actRename.setEnabled( true );

        if ( n instanceof Model ) {
            r.add( m_actAddProperty );
            r.add( m_actImportProperty );
            r.add( m_actNewSspTask );
            Model m = ( Model ) n;
            boolean propertyCapable = ( m.type() != Model.Type.PML );
            m_actAddProperty.setEnabled( propertyCapable );
            m_actImportProperty.setEnabled( propertyCapable );
        }
        if ( n == models() ) {
            r.add( m_actNewDveModel );
            r.add( m_actNewPmlModel );
            r.add( m_actImportModel );
        }
        if ( n == tasks() || n instanceof Property )
            r.add( m_actNewTask );
        if ( n instanceof Task )
            r.add( m_actNewJob );
        if ( n instanceof Job ) {
            r.add( m_actExecuteJob );		
            r.add( m_actAbortJob );		
            Job j = (Job) n;
            m_actExecuteJob.setEnabled( j.state() == Job.State.NotStarted
                    || j.state() == Job.State.Aborted );
            m_actAbortJob.setEnabled( j.state() == Job.State.Queued
                    || j.state() == Job.State.Running );
            boolean touchable = 
                j.state() != Job.State.Queued
                && j.state() != Job.State.Running
                && j.state() != Job.State.Prepared
                && j.state() != Job.State.Aborting;
            m_actRename.setEnabled( touchable );
            m_actDelete.setEnabled( touchable );
        }
        if ( n instanceof Model || n instanceof Property
                || n instanceof Task || n instanceof Job ) {
            r.add( m_actRename );
            r.add( m_actDelete );
        }
        return r;
    }

    public void showNodeMenu( Node n, int x, int y ) {
        Logger.log( 2, "show menu for a node" );
        JPopupMenu mnu = new JPopupMenu();

        for ( Action a : actionList( n ) )
            mnu.add( a );

        mnu.show( tree, x, y );
    }

    /**
     * Ensures that node is visible and selected.
     * @param m Node to show and select.
     */
    public void expandAndSelectTreenode( DefaultMutableTreeNode m )
    {
        if ( m == null ) // ignore
            return;
        TreePath p = new TreePath( m.getPath() );
        this.tree.setSelectionPath( p );
        expandToTreenode( m );
        // this.tree.scrollPathToVisible( p );
    }

    public void selectNode( Node n ) {
        expandAndSelectTreenode( treenode( n ) );
    }

    public boolean treenodeExpanded( DefaultMutableTreeNode m ) {
        return tree.isExpanded( new TreePath( m.getPath() ) );
    }

    public void expandTreenode( DefaultMutableTreeNode m ) {
        if ( m == null ) return;
        tree.expandPath( new TreePath( m.getPath() ) );
    }

    public void setLocalMode( boolean m ) {
        m_actLogout.setEnabled( !m );
    }

    /**
     * Ensures that node is visible.
     * @param m Node to show.
     */
    public void expandToTreenode( DefaultMutableTreeNode m )
    {
        if ( m == null )
            return;
        while ( m != null ) {
            m = (DefaultMutableTreeNode) m.getParent();
            expandTreenode( m );
        }
        // this.tree.scrollPathToVisible( new TreePath( m.getPath() ) );
    }

    public void removeNode( Node o )
    {
        try {
            ( ( DefaultTreeModel ) this.tree.getModel() ).removeNodeFromParent(
                    treenode( o ) );
        } catch ( NoSuchElementException e ) {}
        tree.repaint(); // XXX?
    }

    // panel factory
    NodePanel createPanel( Object o )
    {
        NodePanel ret = new NodePanel();
        if ( o instanceof Model )
            ret = new ModelEditor( client(), (Model) o );
        if ( o instanceof Property ) {
            if ( ((Property) o ).type() == Property.Type.LTL )
                ret = new DvePropertyEditor( client(), (Property) o );
        }
        if ( o instanceof Job )
            ret = new JobEditor( client(), (Job) o );
        if ( o instanceof Task ) {
            Task t = (Task) o;
            ret = new TaskEditor( client(), t );
            /* else
               ret = new TaskViewer( this, t ); */
        }
        if ( o instanceof Cluster )
            ret = new ClusterStatusViewer( this, (Cluster) o );
        if ( o instanceof Profile )
            ret = new ProfileEditor( this, (Profile) o );
        setFontRec( ret, getFont() );
        // etc
        // throw new Exception( "Cannot create Panel for " + o.getClass().getName() );
        return ret;
    }

    protected HashMap< Object, DefaultMutableTreeNode > m_treenodes;
    protected boolean m_syncingTree;

    public DefaultMutableTreeNode treenode( Object o ) {
        return m_treenodes.get( o );
    }

    public DefaultMutableTreeNode treenodeParent( Object o ) {
        return ( DefaultMutableTreeNode ) treenode( o ).getParent();
    }

    public DefaultMutableTreeNode createTreenode( Node o, Object parent )
    {
        DefaultMutableTreeNode p = null, n;

        if ( o.deleted() ) return null;

        if ( parent != null )
            p = m_treenodes.get( parent );
        if ( p == null )
            p = treenode( user() );
        if ( o == currentRoot() )
            p = null;

        n = new DefaultMutableTreeNode( o );

        if ( p != null ) {
            ( ( DefaultTreeModel ) this.tree.getModel() ).insertNodeInto(
                    n, p, p.getChildCount() );
        }
            // this.tree.expandPath( new TreePath( treeNode( m_toplevel ) ) );
        m_treenodes.put( o, n );
        return n;
    }

    static class JobStateInfo {
        protected Job m_job;
        protected String m_state;
        public Job job() { return m_job; }
        public String toString() { return
                job().name()
                + " for " + job().task().name()
                + " changed state to: " + m_state; }
        public JobStateInfo( Job j ) {
            m_job = j;
            m_state = j.stateString();
        }
    }

    public void notifyJobChange( Job j ) {
        DefaultListModel lm = (DefaultListModel) listNotification.getModel();
        JobStateInfo i = new JobStateInfo( j );
        lm.addElement( i );
        listNotification.ensureIndexIsVisible( lm.lastIndexOf( i ) );
    }

    protected void setPanel( NodePanel p ) {
        /* int w = splitTop.getDividerLocation();
        splitTop.setRightComponent( p );
        m_currentPanel = p;
        splitTop.setDividerLocation( w ); */
        m_panelSlave.removeAll();
        m_panelSlave.add( p );
        m_panelSlave.validate();
        m_currentPanel = p;
    }

    // update*
    protected void updatePanel()
    {
        Node n = currentNode();

        try {
            if ( m_displayedNode != n || n.changed() ) {
                Logger.log( 2, "updating panel" );
                if ( m_currentPanel == null || m_currentPanel.update( n ) == false ) {
                    Logger.log( 2, "recreating panel" );
                    setPanel( createPanel( n ) );
                    n.clearChanged();
                }
            }
        } catch (Exception e) {
            Logger.log( e );
        }
        m_displayedNode = n;
    }

    protected void updateToolbar() {
        mainToolbar.removeAll();
        for ( Action a : actionList( currentNode() ) )
            mainToolbar.add( a );
        mainToolbar.validate();
        mainToolbar.repaint();
    }

    private void listNotificationValueChanged( javax.swing.event.ListSelectionEvent evt )
    {
        if ( this.listNotification.getSelectedValue() != null )
        {
            JobStateInfo a = ( JobStateInfo )
                listNotification.getSelectedValue();
            selectNode( a.job() );
        }
    }

    public Model importModel() {

        String[] ext = new String[] { "dve", "mdve", "pml" };
        File f = chooseFile( ext );
        if ( f == null )
            return null;
        Model m = client().importModel( f );
        syncTree();
        selectNode( m );
        return m;
    }

    public Property importProperty( Model m ) {

        String[] ext = new String[] { "ltl" };
        File f = chooseFile( ext );
        if ( f == null )
            return null;
        Property p = client().importProperty( m, f );
        syncTree();
        selectNode( p );
        return p;
    }

    private JScrollPane scrollTree, scrollNotification;
    private JList listNotification;
    private JToolBar mainToolbar, modeToolbar;
    private JPanel m_rightSlave, m_panelSlave;
    private JSplitPane splitNotification, splitWorkarea;
    private JTree tree;
    private JMenuBar menuBar;
    private JToggleButton btnAll, btnClusters, btnTasks, btnModels;

    public Action m_actLogout, m_actQuit, m_actAddProperty, m_actNewProfile,
        m_actNewTask, m_actFontSize, m_actDelete, m_actRename, m_actNewJob, m_actStore,
        m_actExecuteJob, m_actAbortJob, m_actNewDveModel, m_actNewPmlModel,
        m_actImportModel, m_actNewSspTask, m_actImportProperty;

    protected void initActions() {
        
        m_actLogout = new AbstractAction( "Logout" ) {
                public void actionPerformed( ActionEvent evt )
                {
                    if ( client().logout() ) client().login();
                }
            };

        m_actQuit = new AbstractAction( "Quit" ) {
                public void actionPerformed( ActionEvent evt )
                {
                    if ( client().logout() ) System.exit( 0 );
                }
            };

        m_actStore = new AbstractAction( "Store Changes" ) {
                public void actionPerformed( ActionEvent evt )
                {
                    client().storeChanges();
                }
            };

        m_actNewDveModel = new AbstractAction( "New DVE Model" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Model m = client().createModel( Model.Type.DVE );
                    syncTree();
                    selectNode( m );
                }
            };

        m_actNewPmlModel = new AbstractAction( "New ProMeLa Model" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Model m = client().createModel( Model.Type.PML );
                    syncTree();
                    selectNode( m );
                }
            };

        m_actImportModel = new AbstractAction( "Import Model" ) {
                public void actionPerformed( ActionEvent evt ) {
                    importModel();
                }
            };

        m_actImportProperty = new AbstractAction( "Import Property" ) {
                public void actionPerformed( ActionEvent evt ) {
                    importProperty( (Model) currentNode() );
                }
            };

        m_actAddProperty = new AbstractAction( "Add Property" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Logger.log( 4, "add property action fired" );

                    Property p = new Property();
                    p.setType( Property.Type.LTL ); // hack
                    ( (Model) currentNode() ).addProperty( p );
                    
                    createTreenode( p, selectedNode() );
                    selectNode( p );
                    client().renameObject( p, p.canonicalName() );
                    client().storeChanges();
                }
            };

        m_actNewProfile = new AbstractAction( "New Profile" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Profile p = new Profile();
                    user().profiles().addUnique( p );
                    syncTree();
                    selectNode( p );
                    client().storeChanges();
                }
            };

        m_actNewTask = new AbstractAction( "New Task" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Property p = null;
                    if ( selectedNode() instanceof Property )
                        p = (Property) selectedNode();
                    Task t = new Task();
                    if ( p == null ) {
                        Model mod = user().models().any();
                        p = mod.properties().any();
                    }
                    t.setProperty( p );
                    user().tasks().addUnique( t );
                    client().renameObject( t, t.canonicalName() );
                    syncTree();
                    selectNode( t );
                    client().storeChanges();
                }
            };

        m_actNewSspTask = new AbstractAction( "New Task" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Model m = (Model) selectedNode();
                    Property p = (Property) m.properties().get( "State Space" );
                    Task t = new Task();
                    if ( p != null )
                        t.setProperty( p );
                    user().tasks().addUnique( t );
                    client().renameObject( t, t.canonicalName() );
                    syncTree();
                    selectNode( t );
                    client().storeChanges();
                }
            };

        m_actNewJob = new AbstractAction( "Add Job" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Task t = (Task) selectedNode();
                    if ( t == null )
                        return;
                    Job j = new Job();
                    t.addJob( j );
                    client().renameObject( j, j.canonicalName() );
                    syncTree();
                    selectNode( j );
                    client().storeChanges();
                }
            };

        m_actDelete = new AbstractAction( "Delete Item" ) {
                public void actionPerformed( ActionEvent evt ) {
                    deleteObjectConfirm( selectedNode() );
                }
            };

        m_actExecuteJob = new AbstractAction( "Execute Job" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Job j = (Job) selectedNode();
                    if ( j == null ) return;
                    client().executeJob( j );
                    client().storeChanges();
                }
            };

        m_actAbortJob = new AbstractAction( "Abort Job" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Job j = (Job) selectedNode();
                    if ( j == null ) return;
                    client().abortJob( j );
                    client().storeChanges();
                }
            };

        m_actRename = new AbstractAction( "Rename" ) {
                public void actionPerformed( ActionEvent evt ) {
                    String name = (String) JOptionPane.showInputDialog( self(),
                            "Rename object " + selectedNode().name() + " to: ",
                            selectedNode().name() );
                    client().renameObject( selectedNode(), name );
                    client().storeChanges();
                }
            };

        m_actFontSize = new AbstractAction( "Set Font Size" ) {
                public void actionPerformed( ActionEvent evt ) {
                    Object[] b = {10, 12, 14, 16, 18, 20};
                    Object fontSize = JOptionPane.showInputDialog( self(),
                            "Set new font size", "Font size",
                            JOptionPane.QUESTION_MESSAGE, null, b, getFont().getSize() );
                    setFontRec( self(),
                            new Font( getFont().getFontName(),
                                    getFont().getStyle(),
                                    Integer.parseInt( fontSize.toString() ) ) );
                    syncTree();
                }
            };        

    }

    private void initComponents()
    {
        int preferedSizeX = 800;
        int preferedSizeY = 600;
        
        
        splitNotification = new javax.swing.JSplitPane();
        splitWorkarea = new javax.swing.JSplitPane();
        m_panelSlave = new javax.swing.JPanel();
        m_rightSlave = new javax.swing.JPanel();
        scrollTree = new javax.swing.JScrollPane();
        scrollNotification = new javax.swing.JScrollPane();
        listNotification = new javax.swing.JList();
        tree = new javax.swing.JTree();
        mainToolbar = new javax.swing.JToolBar();
        modeToolbar = new javax.swing.JToolBar();
        menuBar = new javax.swing.JMenuBar();

        JMenu mnuDivine = new JMenu( "DiVinE" );
        // mnuDivine.add( m_actNewTask );
        // mnuDivine.add( m_actNewProfile );
        mnuDivine.addSeparator();
        mnuDivine.add( m_actLogout );
        mnuDivine.add( m_actQuit );

        JMenu mnuModels = new JMenu( "Models" );
        mnuModels.add( m_actNewDveModel );
        mnuModels.add( m_actNewPmlModel );
        mnuModels.add( m_actImportModel );

        JMenu mnuView = new JMenu( "View" );
        mnuView.add( m_actFontSize );

        menuBar.add( mnuDivine );
        menuBar.add( mnuModels );
        menuBar.add( mnuView );

        setJMenuBar( menuBar );

        this.setDefaultCloseOperation( javax.swing.WindowConstants.DO_NOTHING_ON_CLOSE);
            
        this.addWindowListener( new WindowAdapter() {
            public void windowClosing(WindowEvent e)
                {
                    if (client().logout()) System.exit(0);
                    else
                    {
                        System.out.println("Connection lost.");
                        System.exit(0);
                    }
                }           
            }   
        );
        
        //setDefaultCloseOperation( javax.swing.WindowConstants.EXIT_ON_CLOSE );
        setTitle( "DiVinE" );


        
        splitNotification.setDividerLocation( preferedSizeY-60 - 20);
        splitNotification.setDividerSize( 5 );
        splitNotification.setOrientation( javax.swing.JSplitPane.VERTICAL_SPLIT );
        splitNotification.setResizeWeight( 0.85 );

        listNotification.setModel( new DefaultListModel() );
        listNotification.addListSelectionListener( new javax.swing.event.ListSelectionListener()
                                              {
                                                  public void valueChanged( javax.swing.event.ListSelectionEvent evt )
                                                  {
                                                      listNotificationValueChanged( evt );
                                                  }
                                              }
                                            );


        scrollNotification.setViewportView( listNotification );
        splitNotification.setBottomComponent( scrollNotification );

        tree.setMinimumSize( null );
        tree.setRootVisible( false );
        tree.setShowsRootHandles( true );
        // tree.setEditable( true );
        tree.addTreeSelectionListener( new javax.swing.event.TreeSelectionListener()
                                       {
                                           public void valueChanged( javax.swing.event.TreeSelectionEvent evt )
                                           {
                                               if (m_syncingTree) // ignore
                                                   return;
                                               updatePanel();
                                               updateToolbar();
                                           }
                                       }
                                     );

        MouseListener ml = new MouseAdapter() {
                public void mousePressed(MouseEvent e) {
                    int selRow = tree.getRowForLocation(e.getX(), e.getY());
                    TreePath selPath = tree.getPathForLocation(e.getX(), e.getY());
                    if ( e.getButton() == MouseEvent.BUTTON3 ) {
                    if ( selRow != -1 ) {
                        tree.setSelectionPath( selPath );
                    }
                    showNodeMenu( currentNode(), e.getX(), e.getY() );
                }
            };
            };

        tree.addMouseListener(ml);

        scrollTree.setViewportView( tree );

        splitWorkarea.setLeftComponent( scrollTree );
        splitWorkarea.setDividerLocation( 250 );
        splitWorkarea.setDividerSize( 5 );

        // top component, not left... *kicks swing*
        splitNotification.setLeftComponent( splitWorkarea );

        getContentPane().add( splitNotification, java.awt.BorderLayout.CENTER );

        modeToolbar.setFloatable( false );
        modeToolbar.setOrientation( JToolBar.VERTICAL );

        m_rightSlave.setLayout( new java.awt.BorderLayout() );
        // getContentPane().add( mainToolbar, java.awt.BorderLayout.NORTH );
        m_rightSlave.add( mainToolbar, java.awt.BorderLayout.NORTH );
        m_rightSlave.add( m_panelSlave, java.awt.BorderLayout.CENTER );
        m_panelSlave.setLayout( new BoxLayout( m_panelSlave, BoxLayout.Y_AXIS ) );

        splitWorkarea.setRightComponent( m_rightSlave );

        btnAll = new javax.swing.JToggleButton();
        btnClusters = new javax.swing.JToggleButton();
        btnModels = new javax.swing.JToggleButton();
        btnTasks = new javax.swing.JToggleButton();

        btnAll.setIcon( new VTextIcon( btnAll, "All", VTextIcon.ROTATE_LEFT ) );
        btnClusters.setIcon( new VTextIcon( btnClusters, "Clusters", VTextIcon.ROTATE_LEFT ) );
        btnModels.setIcon( new VTextIcon( btnModels, "Models", VTextIcon.ROTATE_LEFT ) );
        btnTasks.setIcon( new VTextIcon( btnModels, "Tasks", VTextIcon.ROTATE_LEFT ) );

        modeToolbar.add( btnAll );
        modeToolbar.add( btnModels );
        modeToolbar.add( btnTasks );
        modeToolbar.add( btnClusters );

        getContentPane().add( modeToolbar, java.awt.BorderLayout.WEST );

        class TriggerMode implements ActionListener {
            Mode m;
            TriggerMode( Mode _m ) { m = _m; }
            public void actionPerformed( java.awt.event.ActionEvent evt )
            {
                setMode( m );
            }
        };

        btnAll.addActionListener( new TriggerMode( Mode.All ) );
        btnClusters.addActionListener( new TriggerMode( Mode.Clusters ) );
        btnModels.addActionListener( new TriggerMode( Mode.Models ) );
        btnTasks.addActionListener( new TriggerMode( Mode.Tasks ) );

        // setMinimumSize( new java.awt.Dimension( preferedSizeX, preferedSizeY ) );
        setPreferredSize( new java.awt.Dimension( preferedSizeX , preferedSizeY ) );
        // Dimension screenSize = java.awt.Toolkit.getDefaultToolkit().getScreenSize();
        // setBounds( ( screenSize.width - 790 ) / 2, ( screenSize.height - 550 ) / 2, 790, 550 );
            
    }

    void setMode( Mode m ) {
        m_mode = m;
        btnAll.getModel().setSelected( m == Mode.All );
        btnClusters.getModel().setSelected( m == Mode.Clusters );
        btnModels.getModel().setSelected( m == Mode.Models );
        btnTasks.getModel().setSelected( m == Mode.Tasks );
        syncTree();
    }
}



