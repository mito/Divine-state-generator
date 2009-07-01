// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;
import java.util.Collection;
import java.util.Vector;
import java.awt.*;
import javax.swing.JTabbedPane;

import divine.common.*;

public class JobOutputViewer extends MainWindow.NodePanel
{
    protected Client m_client;
    protected Job m_job;
    protected JTabbedPane m_tabs;
    protected PlainTextViewer m_out, m_debug;
    protected JobReportViewer m_report;
    protected JobLogViewer m_log;
    protected JobCeViewer m_ce;
    public Job job() { return m_job; }
    public Node element() { return job(); }

    public class MonospacedPlainTextViewer extends PlainTextViewer 
    {
	public MonospacedPlainTextViewer( String s, boolean b )
	{
	    super ( s , b );
	}
	
	public void setFont( Font f)
	{
	    Font newFont = new Font("monospaced", f.getStyle(), f.getSize() );
	    super.setFont(newFont);
	}
    }
						   
    
    public JobOutputViewer( Client c, Job j )
    {
        m_client = c;
        setLayout( new java.awt.BorderLayout() );

        add( m_tabs = new JTabbedPane( JTabbedPane.TOP ) );
        m_tabs.add( "Output", m_out = new MonospacedPlainTextViewer( "", false ) );
        m_tabs.add( "Report", m_report = new JobReportViewer( "" ) );
        m_tabs.add( "Counterexample", m_ce = new JobCeViewer( m_client, j ) );
        m_tabs.add( "Realtime Log", m_log = new JobLogViewer( m_client, j ) );
        // m_tabs.add( "Debug", m_debug = new PlainTextViewer( "", false ) );

        update( j );
    }

    public boolean update( Node e ) {
        if ( !(e instanceof Job) )
            return false;

        Job j = m_job = (Job) e;
        Logger.log( 4, "JobOutputViewer updating: " + j.name() );

        m_out.setText( "" );
        // m_log.setText( "" );

        StringBuffer log = new StringBuffer();
        try {
            /* if ( j.output( null ) == null )
                m_debug.setText( "nothing available yet" );
            else
            m_debug.setText( j.output( null ) ); */

            String x = j.stdOut();
            if ( x == null || x.equals( "<null>" ) )
                x = "No output available yet";
            m_out.setText( x );
            
            m_report.setText( j.report() );
	    m_client.mainWindow().setFontRec( m_report, getFont() );
        } catch ( Exception ex ) { Logger.log( ex ); }

        m_log.update( j );

        if ( job().model().type() != Model.Type.PML )  {
            if ( m_tabs.getTabCount() == 3 ) {
                m_tabs.add( m_ce = new JobCeViewer( m_client, j ), 2 );
                m_tabs.setTitleAt( 2, "Counterexample" );
            }
            m_ce.update( j );
        } else
            if ( m_tabs.getTabCount() == 4 )
               m_tabs.removeTabAt( 2 );
        
        return true;
    }
}
