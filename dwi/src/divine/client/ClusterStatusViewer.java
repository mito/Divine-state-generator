package divine.client;

import java.util.*;
import divine.common.*;

import java.awt.*;
import javax.swing.*;

public class ClusterStatusViewer extends MainWindow.NodePanel
{

    protected Cluster m_cluster;
    protected MainWindow m_client;

    protected String reservationTime,oldReservationTime;
    protected String highlightedJob;
    protected ArrayList<String> nodes;
    protected ArrayList<Reservation> resList;

    private javax.swing.JScrollPane scrComputers;
    private javax.swing.JPanel pnlComputers;

    protected class Reservation {
	public int nodeLine;
	public String resId;
	public String jobState;
	public int startPoints;
	public int durationPoints;
	
	public Reservation(String nodeName,
			   String resId1,
			   String jobState1,
			   String start,
			   String duration) {
	    
	    String[] nameToSearch = nodeName.split("\\.");

	    nodeLine = nodes.indexOf(nameToSearch[0]);

	    resId = resId1;
	    jobState = jobState1;
	    startPoints = timeStringToPoints(start);
	    durationPoints = timeStringToPoints(start,duration)-startPoints;		
	};	    
    };

    protected class ClusterGraph extends JPanel {

	public void paint(Graphics g) {
		
	    Graphics2D g2 = (Graphics2D) g;
	    Dimension d = getSize();
	    
	    int lines = nodes.size();
	    int lineHeight = d.height / lines; 
	    
	    g2.setPaint(Color.white);
	    g2.fillRect(0, 0, d.width - 1, d.height - 1);
	    
	    for (int i=0; i<resList.size(); i++)
		{
		    Reservation r = resList.get(i);
		    
		    g2.setPaint(Color.green);
		    
		    if (r.jobState.contains("Running"))
			g2.setPaint(new Color(0,100,0));
		    
		    if (r.jobState.contains("N/A"))
			g2.setPaint(Color.red);			    
		    
		    if (r.resId.contains(highlightedJob))
 			g2.setPaint(Color.blue);
		    
		    int y = (r.nodeLine)*lineHeight+lineHeight/2 - 5;
		    int h = 10;
		    
		    g2.fillRect(r.startPoints,y,
				r.durationPoints,h);
//  		    System.out.print("["+r.startPoints+","+y+","+
//  				     r.durationPoints+","+h+"]");		    
		}
//  	    System.out.println();
	}
	
	public ClusterGraph() {
	    setBackground(Color.white);
	};
    }; 


    protected int  timeStringToPoints(String startTime){
	return timeStringToPoints(startTime,"00:00:00");
    }

    protected int timeStringToPoints(String startTime, String plusTime){
	double hourInPixels=10;
	int daySeparatorInPixels=0;
	double result=0; //in minutes
	String[] start = startTime.split(":"); // may have 3 to 4items (hours:minutes:seconds)
	String[] plus = plusTime.split(":"); // may have 3 to 4 items (days:hours:...)
	
	Boolean startIsNegative = startTime.contains("-");
	Boolean plusIsNegative = plusTime.contains("-");
	
	int a,b,c = 0;
	
	a = new Integer(start[0]).intValue();
	b = new Integer(start[1]).intValue();
	c = new Integer(start[2]).intValue();
	
	if (startIsNegative)
	    {
		if (start.length==4)		    
		    {
			result = a*24*60 - b*60 - c;
		    }
		else
		    {
			result = a*60 - b;
		    }
	    }
	else
	    {
		if (start.length==4)		    
		    {
			result = a*24*60 + b*60 +c;
		    }
		else
		    {
			result = a*60 + b;
		    }
	    }
	
	if (plus.length == 4)
	    {
		a = new Integer(plus[0]).intValue();
		b = new Integer(plus[1]).intValue();
		c = new Integer(plus[2]).intValue();		    
		if (plusIsNegative)
		    result += a*24*60 - b*60 - c;
		else
		    result += a*24*60 + b*60 + c;
	    }
	else
	    {
		b = new Integer(plus[0]).intValue();
		c = new Integer(plus[1]).intValue();
		if (plusIsNegative)
		    result += b*60 - c;
		else
		    result += b*60 + c;
	    }	    
	
	if (result < 0) return 0;
	
	int days = (int) result/(24*60);
	result = (hourInPixels*(result/60));	    
	return days*daySeparatorInPixels + (int) result;
    }
       
    public ClusterStatusViewer( MainWindow c, Cluster cl )
    {
        m_cluster = cl;
        m_client = c;
        initComponents();
        update( cl );
    }

    public void setHighlightedJob(String s)
    {
	if (s.length() == 0)
	    s="none";
	highlightedJob = s;
    }

    public String updateReservations() 
    {
	String[] all = (m_cluster.reservations()).split("\n");
	String[] line;
	
	if (!(all[0].contains("reservations")))
	    return "";

	line = all[0].split(" ");
	resList.clear();
 	reservationTime=line[line.length-1];
	
  	for (int i = 1; i < all.length; i++)
	    {
		if (all[i].contains("Job id"))
		    {
			String showq = "";
			for (int j = i; j< all.length; j++)
			    {
				showq = showq + all[j] + "\n";
			    }
			return showq;
		    }
		line = all[i].split(" +");
		if (line.length > 7)
		    if (!line[1].contains("NodeName"))
			{
			Reservation res = new Reservation(line[1],line[3],line[4],line[6],line[7]);
			resList.add( res );	
		    }
	    }
	return "";
    };

    public class MonospacedTextArea extends JTextArea {
	public MonospacedTextArea ( String s )
	{
	    super(s);
	}
	public void setFont( Font f )
	{
	    Font newFont = new Font("monospaced", f.getStyle(), f.getSize() );
	    super.setFont(newFont);
	}
    }
    
    public boolean update( Node e )
    {		
        if ( !( e instanceof Cluster ) )
            return false;
        m_cluster = (Cluster) e;
	
	MonospacedTextArea showqTextArea = new MonospacedTextArea( updateReservations() );
	showqTextArea.setBackground( new Color(0xef,0xef,0xef) );
        
        java.text.DateFormat df;
        String timeString;
        String dateString;

	if (reservationTime == null)
	    {
	      	JLabel heading = new JLabel();
		heading.setText( "Status of "+m_cluster.name()+" cluster is not available yet. Please wait." );
		
		pnlComputers.removeAll();
		pnlComputers.add( heading );
		pnlComputers.add( new JPanel() );
	    }
	else
	    {
                if (reservationTime.equals(oldReservationTime))
                {
                    return false;
                }
                oldReservationTime = reservationTime;
                df = java.text.DateFormat.getTimeInstance();
                timeString = df.format((new java.util.Date()).getTime());
                df = java.text.DateFormat.getDateInstance();
                dateString = df.format((new java.util.Date()).getTime());

		JLabel heading = new JLabel();
		heading.setText( m_cluster.name()+" cluster status at " +  reservationTime +
				 " (current time: "+timeString+")");

		pnlComputers.removeAll();
		pnlComputers.add( heading );
		pnlComputers.add( new JPanel() );

		JPanel panel = new JPanel();
		panel.setLayout( new BorderLayout());
		JPanel names = new JPanel();
		names.setLayout( new GridLayout(nodes.size(),1) );
		panel.add(names,BorderLayout.WEST);
		TreeSet< String > keys = new TreeSet();
		keys.addAll( m_cluster.computers().keySet() );
		for ( String k : keys ) {
		    Computer i = m_cluster.computers().get( k );
		    JLabel label = new JLabel(i.name()+":");
		    names.add(label);
		}	    
		ClusterGraph cg = new ClusterGraph();
		panel.add(cg);
		
		pnlComputers.add( panel );
		
		pnlComputers.add( new JPanel() );
		pnlComputers.add( showqTextArea );		
	    }
	
	MainWindow.setFontRec( this, getFont() );
	pnlComputers.updateUI();
	
	scrComputers.repaint();
	return true;
    }

    private void initComponents()
    {
        pnlComputers = new JPanel();
        scrComputers = new JScrollPane();

        setLayout( new javax.swing.BoxLayout( this, javax.swing.BoxLayout.Y_AXIS ) );

        pnlComputers.setLayout( new javax.swing.BoxLayout(
                        pnlComputers, javax.swing.BoxLayout.Y_AXIS ) );
        scrComputers.setViewportView( pnlComputers );
        add( scrComputers );

	nodes = new ArrayList<String>();

	TreeSet< String > keys = new TreeSet();
	keys.addAll( m_cluster.computers().keySet() );
	for ( String k : keys ) {
	    Computer c = m_cluster.computers().get( k );
	    nodes.add(c.name());
	}					


	resList = new ArrayList<Reservation>();
	highlightedJob = new String("none");
	updateReservations();
    }

}






