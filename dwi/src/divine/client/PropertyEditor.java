package divine.client;
import divine.common.*;
import java.util.*;
import javax.swing.*;
import java.awt.Component;

/**
 * Panel used to display and change the content of the property.
 * @author  xforejt
 */
public class PropertyEditor
    extends MainWindow.NodePanel
{
    private MainWindow.NodePanel mainPanel;
    private Property m_property;
    protected Client m_client;
    protected boolean m_inUpdate;
    public Property property() { return m_property; }

    public PropertyEditor( Client c, Property p )
    {
        m_inUpdate = false;
        m_client = c;
        initComponents();
        chooser = new JFileChooser();
        update( p );
    }

    public boolean update( Node elem )
    {
        if ( !( elem instanceof Property ) )
            return false;

        m_inUpdate = true;
        Property p = ( Property ) elem;

        boolean changed = false;
        int idx = 0;
        if ( m_property != p ) {
            m_property = p;
            changed = true;
            String[] s;
            if ( property().model().type() == Model.Type.DVE ) {
                s = new String[] {"State Space Generation",
                                  "Safety Properties",
                                  "Model with Property Process",
                                  "LTL Property" };
            } else {
                s = new String[] { "State Space Generation", "Model with Neverclaim" };
            }
            cmbPropertyType.setModel( new DefaultComboBoxModel( s ) );
        }

        Property.Type t = property().type();
        if ( t == Property.Type.StateSpace ) idx = 0;
        if ( t == Property.Type.Safety ) idx = 1;
        if ( t == Property.Type.PropertyProcess ) idx = 2;
        if ( t == Property.Type.LTL ) idx = 3;

        if ( changed || idx != cmbPropertyType.getSelectedIndex() )
            cmbPropertyType.setSelectedIndex( idx );

        m_inUpdate = false;
        return true;
    }

    private void cmbPropertyTypeActionPerformed( java.awt.event.ActionEvent evt )
    {
        MainWindow.NodePanel pnl = null;
        switch ( this.cmbPropertyType.getSelectedIndex() )
        {
        case 0:
            property().setType( Property.Type.StateSpace );
            pnl = new MainWindow.NodePanel();
            break;
        case 1:
            property().setType( Property.Type.Safety );
            pnl = new MainWindow.NodePanel();
            break;
        case 2:
            property().setType( Property.Type.PropertyProcess );
            pnl = new MainWindow.NodePanel();
            break;
        case 3:
            property().setType( Property.Type.LTL );
            pnl = new DvePropertyEditor( m_client, property() );
            break;
        }

        if ( !m_inUpdate )
            m_client.renameObject( property(), property().canonicalName() );

        if ( pnl != null )
        {
            setMainPanel( pnl );
        }

        m_client.mainWindow().repaint(); // ???
        manageButtons();

        /* for ( Task t : property().tasks().values() ) {
            if ( !t.mutable() )
                m_client.setEnabledRec( this, false );
                } */
    }

    private void manageButtons()
    {
        boolean val = property().type() == Property.Type.LTL;
        this.btnFileBrowser.setEnabled( val );
        // cmbPropertyType.setEnabled( property().tasks().isEmpty() );
        // btnCheckSyntax.setEnabled( false );
    }

    public void setEnabled( boolean e ) {
        super.setEnabled( e );
        btnAddTask.setEnabled( true );
        btnCheckSyntax.setEnabled( true );
    }

    /**
     * Sets main component of this panel and places it in proper place in layout.
     * Called when type of property changes.
     */
    private void setMainPanel( MainWindow.NodePanel pnl )
    {
        if ( this.mainPanel != null )
        {
            this.remove( this.mainPanel );
        }
        this.mainPanel = pnl;
        this.add( pnl, java.awt.BorderLayout.CENTER );
        //((java.awt.BorderLayout)this.getLayout()).invalidateLayout(this);
        this.validate();
        this.repaint();
    }

    protected void messageDialog( String t, String c, int type ) {
        JOptionPane.showMessageDialog( this, t, c, type );
    }

    private JFileChooser chooser;

    private void btnFileBrowserActionPerformed( java.awt.event.ActionEvent evt )
    {
        String[] ext = new String[ 1 ];
        ext[ 0 ] = "ltl";
        //ext[1] = "mdve";
        String content = m_client.mainWindow().readFile( this.chooser, ext );
        if ( content.length() > 0 )
        {
            property().setText( content );
            mainPanel.update( property() );
            // XXX following font update may be redundant
            m_client.mainWindow().setFontRec( this, getFont() );
        }
    }

    private void btnCheckSyntaxAction( java.awt.event.ActionEvent evt ) {
        if ( m_client.checkSyntax( null, property() ) != null ) {
            JOptionPane.showMessageDialog( this,
                    "Property or model is not valid",
                    "Invalid property/model", JOptionPane.ERROR_MESSAGE );
        } else {
            JOptionPane.showMessageDialog( this,
                    "Both Property and model are valid",
                    "Valid", JOptionPane.INFORMATION_MESSAGE );
        }
    }

    private void btnAddTaskActionPerformed( java.awt.event.ActionEvent evt )
    {
       /* if ( !m_client.ensurePropertyValid( property() ) )
          return; */
        Logger.log( 4, "adding task" );
        /* Task t = new Task();
        t.setAlgorithmsAvailable( m_client.algorithms() );
        t.setClusters( m_client.clusters() );

        property().addTask( t );
        m_client.syncTree();
        m_client.expandAndSelectNode( m_client.treeNode( t ) );
        m_client.renameObject( t, t.canonicalName() );

        Logger.log( 4, "task added" ); */
    }

    // GENERATED stuff follows

    /** This method is called from within the constructor to
     * initialize the form.  WARNING: Do NOT modify this code. The
     * content of this method is always regenerated by the Form
     * Editor.
     */ 
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents()
    {
        jToolBar1 = new javax.swing.JToolBar();
        cmbPropertyType = new javax.swing.JComboBox();
        btnAddTask = new javax.swing.JButton();
        btnFileBrowser = new javax.swing.JButton();
        btnCheckSyntax = new javax.swing.JButton();

        setLayout( new java.awt.BorderLayout() );

        cmbPropertyType.addActionListener( new java.awt.event.ActionListener()
                                           {
                                               public void actionPerformed( java.awt.event.ActionEvent evt )
                                               {
                                                   cmbPropertyTypeActionPerformed( evt );
                                               }
                                           }
                                         );

        jToolBar1.add( cmbPropertyType );

        btnAddTask.setText( "Add Task" );
        btnAddTask.addActionListener( new java.awt.event.ActionListener()
                                      {
                                          public void actionPerformed( java.awt.event.ActionEvent evt )
                                          {
                                              btnAddTaskActionPerformed( evt );
                                          }
                                      }
                                    );

        jToolBar1.add( btnAddTask );

        btnFileBrowser.setText( "From file" );
        btnFileBrowser.addActionListener( new java.awt.event.ActionListener()
                                          {
                                              public void actionPerformed( java.awt.event.ActionEvent evt )
                                              {
                                                  btnFileBrowserActionPerformed( evt );
                                              }
                                          }
                                        );

        jToolBar1.add( btnFileBrowser );

        btnCheckSyntax.setText( "Check syntax" );
        btnCheckSyntax.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent evt )
                {
                    btnCheckSyntaxAction( evt );
                }
            } );

        jToolBar1.add( btnCheckSyntax );

        add
            ( jToolBar1, java.awt.BorderLayout.NORTH );

    }
    // </editor-fold>//GEN-END:initComponents

    private javax.swing.JButton btnAddTask;
    private javax.swing.JButton btnCheckSyntax;
    private javax.swing.JButton btnFileBrowser;
    private javax.swing.JComboBox cmbPropertyType;
    private javax.swing.JToolBar jToolBar1;
}
