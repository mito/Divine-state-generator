// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;

import java.util.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import divine.common.*;
import java.sql.Time; // why it's under sql is beyond me...

public class JobEditor
    extends MainWindow.NodePanel
{
    protected Job m_job;
    protected Client m_client;
    protected int m_inUpdate = 0;

    public User user() { return job().user(); }
    public Job job() { return m_job; }
    public Task task() { return job().task(); }

    public JobEditor( Client client, Job t )
    {
        m_client = client;
        initComponents();

        update( t );
    }

    public boolean update( Node e ) {
        if ( ! (e instanceof Job) ) 
            return false;

        Job j = ( Job ) e;
        if ( j.task() == null || j.task().algorithm() == null
             || j.task().property() == null || j.cluster() == null ) return false;

        Logger.log( 4, "JobEditor updating for " + j.name() );

        m_inUpdate ++;

        if ( m_job != j ) {
            m_job = j;
            updateTask();
            updateClusterList();
        }

        Color col = new Color( 0 );
        if ( m_job.state() == Job.State.Running )
            col = new Color( 0xafaf00 );
        else if ( m_job.state() == Job.State.Aborted )
            col = new Color( 0xff0000 );
        else if ( m_job.state() == Job.State.Finished ) {
            col = new Color( 0x00ff00 );
        }
        m_lState.setForeground( col );
        m_lState.setText( m_job.stateString() );

        m_comboCluster.setEnabled( j.editable() );
        m_spinMachines.setEnabled( j.editable() );
        m_spinWalltime.setEnabled( j.editable() );

        // wants milliseconds and is 60 minutes off (who knows why
        // gets a cookie)
        m_walltime.setValue( new Time( ( j.walltime() - 60 ) * 60000 ) );

        updateCluster();
        m_output.update( e );
        m_inUpdate --;
        return true;
    }

    public void updateTask() {
        if ( job() == null || task() == null )
            m_lTask.setText( "task data not available" );
        else
            m_lTask.setText( task().name() + " using algorithm: " + task().algorithm().name() );
    }

    public void setLineHeight( int lh ) {
        int pc = 0; // XXX parameter count
        setMaximumSize( new Dimension( 65000, lh * ( 1 + pc ) ) );
    }

    protected void updateAvailableLabel() {
        String x = spinMachinesValue() > 1 ? "nodes" : " node";
        int avail = 0;
        if ( job() != null && job().cluster() != null )
            avail = job().cluster().leastLoaded( 65000 ).size();
        // Logger.log( 3, "cluster: " + algorithm().cluster().name() );
        m_lAvailable.setText( " " + x + " (" + avail + " available) " );
    }

    protected void updateCluster() {
        int mach = job().machines(); // cache, since evt will kill it
        Logger.log( 4, "algorithm machines: " + mach );
        SpinnerNumberModel m = (SpinnerNumberModel) m_spinMachines.getModel();
        m.setMinimum( 1 );
        m.setMaximum( job().cluster().computerCount() );
        m_spinMachines.setValue( mach );
        updateAvailableLabel();
        m_comboCluster.setSelectedItem( job().cluster().name() );
    }

    public void updateClusterList() {
        m_inUpdate ++;
        TreeSet< String > cl = new TreeSet();
        if ( user().clusters() != null ) {
            cl.addAll( user().clusters().keySet() );
        }
        m_comboCluster.setModel( new DefaultComboBoxModel( cl.toArray() ) );
        m_inUpdate --;
    }

    public void rename() {
        if ( m_inUpdate > 0 ) return;
        m_client.renameObject( job(), job().canonicalName() );
    }

    protected int spinMachinesValue() {
        return ( Integer ) m_spinMachines.getValue();
    }

    private JComboBox m_comboCluster;
    private JSpinner m_spinMachines, m_spinWalltime;
    private JLabel m_lTask, m_lAvailable, m_lCluster, m_lUsing, m_lState, m_lWalltime;
    private JPanel m_pCluster, m_pInfo, m_pPbs;
    private JobOutputViewer m_output;
    private SpinnerDateModel m_walltime;

    private void initComponents()
    {
        m_comboCluster = new javax.swing.JComboBox();
        m_spinMachines = new javax.swing.JSpinner();
        m_lState = new JLabel( "" );
        m_lTask = new JLabel( "Task" );
        m_lCluster = new JLabel( "Run on cluster: " );
        m_lUsing = new JLabel( " using " );
        m_lAvailable = new JLabel( "foobar" );
        m_output = new JobOutputViewer( m_client, m_job );

        m_pPbs = new JPanel();
        m_pInfo = new JPanel();
        m_pCluster = new JPanel();

        m_lWalltime = new JLabel( "Maximum walltime: " );
        m_spinWalltime = new JSpinner();
        m_spinWalltime.setModel( m_walltime = new SpinnerDateModel() );
        m_spinWalltime.setEditor( new JSpinner.DateEditor( m_spinWalltime, "HH:mm" ) );

        m_pCluster.setLayout( new BoxLayout( m_pCluster, BoxLayout.X_AXIS ) );
        setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        m_comboCluster.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent evt )
                {
                    if ( m_inUpdate > 0 ) return;
                    String sel = (String) m_comboCluster.getSelectedItem();
                    Logger.log( 4, "selected item: " + sel );
                    job().setCluster( sel );
                    rename();
                }
            } );

        m_spinMachines.addChangeListener( new ChangeListener() {
                public void stateChanged( ChangeEvent evt ) {
                    if ( m_inUpdate > 0 ) return;
                    job().setMachines( spinMachinesValue() );
                    updateAvailableLabel();
                    rename();
                }
            } );

        m_spinWalltime.addChangeListener( new ChangeListener() {
                public void stateChanged( ChangeEvent evt ) {
                    if ( m_inUpdate > 0 ) return;
                    // gives milliseconds, so divide by 60000
                    job().setWalltime( (int) (m_walltime.getDate().getTime() / 60000) + 60 );
                }
            } );

        updateAvailableLabel();

        // m_lTask.setBorder( BorderFactory.createEmptyBorder( 3, 3, 0, 3 ) );
        m_pCluster.setBorder( BorderFactory.createEmptyBorder( 0, 3, 3, 3 ) );

        m_pPbs.add( m_lWalltime );
        m_pPbs.add( m_spinWalltime );
        m_pInfo.add( m_lState );
        m_pInfo.add( m_lTask );
        add( m_pInfo );
        add( m_pCluster );
        add( m_pPbs );
        m_pCluster.add( m_lCluster );
        m_pCluster.add( m_comboCluster );
        m_pCluster.add( m_lUsing );
        m_pCluster.add( m_spinMachines );
        m_pCluster.add( m_lAvailable );
        add( m_output );
    }

}
