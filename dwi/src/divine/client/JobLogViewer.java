package divine.client;

import divine.common.*;
import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.JTableHeader;

import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.ListSelectionEvent;
import java.util.List;
import java.util.Arrays;
import java.awt.geom.*;

/**
  Derived from code by xforejt
 */
public class JobLogViewer extends MainWindow.NodePanel
{
    // private GraphPanel pnlGraph;
    private Job m_job;
    private javax.swing.Timer timer;
    /** Creates new form RecentLogPanel */
    public JobLogViewer( Client c, Job j ) {
        initComponents();
        update( j );
    }

    public boolean update( Node n )
    {
        if ( !(n instanceof Job) )
            return false;

        Job j = m_job = (Job) n;

        /* if ( j.state() == Job.State.NotStarted )
           return ; */

        String log = j.lastLog();
        List< String > slice = Arrays.asList( log.split( "\n" ) );
        if ( slice.size() > 3 )
        {
            slice = slice.subList( 3, slice.size() ); // strip the header... uhm
            String[][] data = new String[ slice.size() ][ slice.get( 0 ).split( "\t" ).length ];
            int i = 0;
            for( String r : slice ) {
                data[ i ] = r.split( "\t" );
                ++ i;
            }

            m_table.setModel( new DefaultTableModel( data, columns ) );
        } else {
            m_table.setModel( new DefaultTableModel( new Object[][] {}, new String[] {} ) );
        }

        return true;
    }

    // XXX read from the actual logfile
    private final String[] columns = { "Time", "User time", "Sys time",
                                       "Idle", "Memory", "States", "Sent",
                                       "Recieved", "Barrier", "Packets", "Cmatrix" };

    private void initComponents()
    {
        m_scroll = new javax.swing.JScrollPane();
        m_table = new javax.swing.JTable();

        setLayout( new java.awt.GridLayout( 1, 0 ) );

        m_table.setModel( new DefaultTableModel( new Object[][] {}, new String[] {} ) );
        m_table.setCellSelectionEnabled( false );
        m_table.setSelectionMode( ListSelectionModel.SINGLE_SELECTION );

        m_scroll.setViewportView( m_table );
        add( m_scroll );

    }


    private javax.swing.JScrollPane m_scroll;
    private javax.swing.JTable m_table;
}
