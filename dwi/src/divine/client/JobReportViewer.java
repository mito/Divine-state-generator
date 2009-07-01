package divine.client;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.*;
import javax.swing.event.*;
import javax.swing.*;
import java.util.*;
import divine.common.Logger;


public class JobReportViewer extends javax.swing.JPanel
{
  

    public class DistrGraph extends JPanel
    {
	
	private class MinmaxPanel extends JPanel
	{
	    private Font localFont;		

	    public void paint (java.awt.Graphics g)
	    {
		Graphics2D g2 = (Graphics2D) g;
		Dimension d = getSize();
		int sep = getSeparator();
		    		    
  		g2.setPaint(new Color(0xdddddd));
  		g2.fillRect(0, 0, d.width - 1, d.height - 1);
		g2.setFont(localFont);
		FontMetrics fm = getFontMetrics(localFont);
		int text_h = fm.getHeight(); 

		int max_h = 0;
		int avg_h = 0;
		int min_h = 0;

		String max_s = "0";
		String min_s = "0";
		String avg_s = "0";

		if (nodes != null &&  nodes.size() != 0 && maximumValue != 0)
		    { 
			max_h = d.height - 4*sep - 2*text_h;
			avg_h = (int) (max_h * ( (1.0 * getAverage()) / maximumValue ));
			min_h = (int) (max_h * ( (1.0 * minimumValue) / maximumValue ));

			max_s = new String().valueOf( maximumValue );
			min_s = new String().valueOf( minimumValue );
			avg_s = new String().valueOf( getAverage() );
		    }

		    
		int text_w_max = ( fm.stringWidth("Max") > fm.stringWidth( max_s ) ?
				   fm.stringWidth("Max") : fm.stringWidth( max_s ) );

		int text_w_avg = ( fm.stringWidth("Avg") > fm.stringWidth( avg_s ) ?
				   fm.stringWidth("Avg") : fm.stringWidth( avg_s ) );

		int text_w_min = ( fm.stringWidth("Min") > fm.stringWidth( min_s ) ?
				   fm.stringWidth("Min") : fm.stringWidth( min_s ) );


		    
		g2.setPaint(Color.black);
		g2.drawString("Max",
				sep + text_w_max/2 - fm.stringWidth("Max")/2,
				d.height - sep);
		g2.drawString(max_s,
			      sep + text_w_max/2 - fm.stringWidth(max_s)/2,
			      d.height - 3*sep - text_h - max_h );

		g2.drawString("Avg",
			      2*sep + text_w_max + text_w_avg/2 -fm.stringWidth("Avg")/2,
			      d.height - sep );
		g2.drawString(avg_s,
			      2*sep + text_w_max + text_w_avg/2 -fm.stringWidth(avg_s)/2,
			      d.height - 3*sep - text_h - avg_h );

		g2.drawString("Min",
			      3*sep + text_w_max + text_w_avg + text_w_min/2 - 
			      fm.stringWidth("Min")/2, 
			      d.height - sep );		    
		g2.drawString(min_s,
			      3*sep + text_w_max + text_w_avg + text_w_min/2 - 
			      fm.stringWidth(min_s)/2, 
			      d.height - 3*sep - text_h - min_h );
		    
		int bar_w = getBarWidth();
		int bar_sep = (d.width - 4*sep - 3*bar_w)/6;
		g2.setPaint(Color.red);
  		g2.fillRect(sep+bar_sep,
 			    d.height - 2*sep - text_h - max_h,
  			    bar_w,max_h);
		g2.fillRect(sep+text_w_max+sep+bar_sep,
			    d.height - 2*sep - text_h - avg_h,
			    bar_w,avg_h);
		g2.fillRect(sep+text_w_max+sep+text_w_avg+sep+bar_sep,
			    d.height - 2*sep - text_h - min_h,
			    bar_w,min_h);
	    }
		
	    public int requiredWidth()
	    {
		FontMetrics fm = getFontMetrics(localFont);

		String max_s = new String().valueOf( maximumValue );
		String min_s = new String().valueOf( minimumValue );
		String avg_s = new String().valueOf( getAverage() );

		int text_w_max = ( fm.stringWidth("Max") > fm.stringWidth( max_s ) ?
				   fm.stringWidth("Max") : fm.stringWidth( max_s ) );

		int text_w_avg = ( fm.stringWidth("Avg") > fm.stringWidth( avg_s ) ?
				   fm.stringWidth("Avg") : fm.stringWidth( avg_s ) );

		int text_w_min = ( fm.stringWidth("Min") > fm.stringWidth( min_s ) ?
				   fm.stringWidth("Min") : fm.stringWidth( min_s ) );
		//System.out.println(4*sep+text_w_max+text_w_avg+text_w_min);
		return (4*getSeparator()+text_w_max+text_w_avg+text_w_min);
	    }
		
	    public MinmaxPanel()
	    {
		localFont = new Font("monospaced", Font.PLAIN, 10);
		setBorder(new javax.swing.border.EmptyBorder(getUserHeight()-10,
							     requiredWidth()-10,
							     0, 0));
	    }
	}



	private class GraphPanel extends JPanel
	{
	    private Font localFont;

	    public void paint (java.awt.Graphics g)
	    {
		Graphics2D g2 = (Graphics2D) g;
		Dimension d = getSize();
		int sep = getSeparator();
		    		    
  		g2.setPaint(new Color(0xdddddd));
  		g2.fillRect(0, 0, d.width - 1, d.height - 1);
		
		g2.setFont(localFont);
		FontMetrics fm = getFontMetrics(localFont);

		int sum = sep;
		int bar_w = getBarWidth();
		Set set = nodes.keySet();
		Iterator iterator = set.iterator();
		while( iterator.hasNext() ) {
		    String s = (String)iterator.next(); 
		    int v = ((Integer)nodes.get( s )).intValue();
		    String v_string = new String().valueOf(v);
		    int text_w = ( fm.stringWidth(s) > fm.stringWidth(v_string) ? 
				   fm.stringWidth(s) : fm.stringWidth(v_string) );
		    int text_h = fm.getHeight();
		    g2.setPaint(Color.black);
		    g2.drawString( s, sum + text_w/2 - fm.stringWidth(s)/2, 
				   d.height - sep);
		    int bar_h = ( maximumValue == 0 ? 
				  0 : (d.height - 4*sep - 2*text_h)*v/maximumValue);
		    g2.drawString( v_string, sum + text_w/2 - fm.stringWidth(v_string)/2,
				   d.height - 3*sep - text_h - bar_h );
		    g2.setPaint(Color.blue);
		    g2.fillRect(sum + (text_w)/2 - (bar_w/2),				
				d.height - 2*sep - text_h - bar_h,
				bar_w,
				bar_h );
		    sum = sum + text_w + sep;
		} 		    

	    }

	    public int requiredWidth()
	    {
		if (nodes == null || nodes.size() == 0)
		    {
			return 0;
		    }
		FontMetrics fm = getFontMetrics(localFont);
		int sum = getSeparator();
		Set set = nodes.keySet();
		Iterator iterator = set.iterator();
		while( iterator.hasNext() ) {
		    String s = (String)iterator.next();
		    int v = ((Integer)nodes.get( s )).intValue();
		    String v_string = new String().valueOf(v);
		    sum = sum + 
			( fm.stringWidth(s) > fm.stringWidth(v_string) ? 
			  fm.stringWidth(s) : fm.stringWidth(v_string) );
		    sum = sum + getSeparator();
		} 		    
		return sum;

	    }
	    public GraphPanel()
	    {
		localFont = new Font("monospaced", Font.PLAIN, 10);
		setBorder(new javax.swing.border.EmptyBorder(getUserHeight()-10,
							     requiredWidth()-10,
							     0, 0));
	    }
	}


	private int barWidth;
	private int separator;
	private GraphPanel graphPanel;
	private MinmaxPanel minmaxPanel;
	private JLabel aboveLabel;
	private JLabel belowLabel;
	private Boolean showMinmaxPanel;
	private int maximumValue;
	private int minimumValue;
	private int userHeight;
	private java.util.TreeMap<String,Integer> nodes;


	private void init()
	{
	    setBarWidth(10);
	    setSeparator(5);
	    setUserHeight(150);
	    setLayout( new BorderLayout() );

	    nodes = new java.util.TreeMap<String,Integer>();
	    maximumValue = 0;
	    minimumValue = 0;

	    aboveLabel = new JLabel("Graph of distribution.");
	    add( aboveLabel, new BorderLayout().NORTH );

	    belowLabel = new JLabel("DiVinE - distrGraph");
	    add( belowLabel, new BorderLayout().SOUTH );

	    showMinmaxPanel = true;
	    minmaxPanel = new MinmaxPanel();
	    add( minmaxPanel, new BorderLayout().WEST );

	    graphPanel = new GraphPanel();
	    add( graphPanel, new BorderLayout().CENTER );
		
	}

	public DistrGraph()
	{
	    init();
	}

	public DistrGraph(String above, String below)
	{
	    init();
	    setAboveText(above);
	    setBelowText(below);		
	}	    

	public void setAboveText(String newValue )
	{
	    if (aboveLabel.getText().equals(newValue))
		{
		    return;
		}
	    else
		{
		    aboveLabel.setText( newValue );
		}
	}

	public void setBelowText(String newValue )
	{
	    if (belowLabel.getText().equals( newValue ))
		{
		    return;
		}
	    else
		{
		    belowLabel.setText( newValue );
		}
	}


	public void setBarWidth(int newValue)
	{
	    barWidth = newValue;
	}
	
	public int getBarWidth()
	{
	    return barWidth;
	}

	public void setSeparator(int newValue)
	{
	    separator = newValue;
	}
	
	public int getSeparator()
	{
	    return separator;
	}

	public void setUserHeight(int newValue)
	{
	    userHeight = newValue;
	}
	
	public int getUserHeight()
	{
	    return userHeight;
	}

	public void addNode(String name, int value)
	{
	    if (nodes.size() == 0)
		{
		    minimumValue = value;
		    maximumValue = value;
		}

	    nodes.put(name,(Integer) value);
		
	    if (value < minimumValue)
		minimumValue = value;
		
	    if (value > maximumValue)
		maximumValue = value;
		
	    remove ( minmaxPanel );
	    minmaxPanel = new MinmaxPanel();
	    add ( minmaxPanel, new BorderLayout().WEST );

	    remove ( graphPanel );
	    graphPanel = new GraphPanel();
	    add ( graphPanel, new BorderLayout().CENTER );
		
	}
	    
	public void hideMinmaxPanel()
	{
	    if (showMinmaxPanel)
		{
		    remove(minmaxPanel);
		    showMinmaxPanel = false;
		}
	}
	    
	public void showMinmaxPanel()
	{
	    if (!showMinmaxPanel)
		{
		    add(minmaxPanel, BorderLayout.WEST );
		    showMinmaxPanel = true;
		}
	}

	private int getAverage()
	{
	    if (nodes == null || nodes.size() == 0)
		{
		    return 0;
		}
	    else
		{
		    int sum = 0;
		    Set set = nodes.keySet();
		    Iterator iterator = set.iterator();
		    while( iterator.hasNext() ) {
			String s = (String)iterator.next(); 
			sum = sum + ((Integer)nodes.get( s )).intValue();
		    } 		    
		    return sum/nodes.size();
		}
	}
    }


    public class ScrPanel extends JPanel implements Scrollable
    {
	public Dimension getPreferredScrollableViewportSize()
	{
	    Dimension d = getPreferredSize();
	    //System.out.println(d.width+","+d.height);
	    return d;	    
	}

	public int getScrollableBlockIncrement(Rectangle visibleRect, int orientation, int direction) 
	{
	    return 10;
	}

	public boolean getScrollableTracksViewportHeight() 
	{
	    return false;
	}

	public boolean getScrollableTracksViewportWidth() 
	{
	    return false;
	}

	public int getScrollableUnitIncrement(Rectangle visibleRect, int orientation, int direction) 
	{
	    return 50;
	}
    }


    private String localReport;
    private JScrollPane scrollPane;
    private TreeMap<String,String> nodeNames;
    

    public JobReportViewer( String content )
    {	
	localReport = new String("nonsence report");
 	setLayout( new BorderLayout() );
	scrollPane = new JScrollPane(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
				     JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
	add( scrollPane, BorderLayout.CENTER );
	setText( content );
    }


    public void setText( String report ) 
    {	
	ScrPanel panel = new ScrPanel();

	
	if (localReport.equals(report))
	    {
		return;
	    }		
	localReport = report;

	if ( report == null || report.equals("<null>") || report.equals("") )
	    {		  

		JPanel pan = new JPanel();
		pan.setLayout(new java.awt.BorderLayout() );		
		pan.add ( new JLabel("Job has not finished successfully, report is unavailable."),
			  java.awt.BorderLayout.NORTH );
		scrollPane.setViewportView( pan );
		return;
	    }
	else
	    {				
		
		String[] all = report.split("\n");
		int problem = 0;  // 1 - SSGen, 2 - Safety, 3 - LTLMC
		int numberNodes = 0;
		String algorithmCommand = "";
		String reportTimeAndDate = "";
                String transNumber="???";
                String statesNumber="???";
		String crossNumber="0";
                String runTime="0";
		boolean ce = false;

		JLabel label;
		JPanel resultPanel = new JPanel();
		JPanel leftRP = new JPanel();
		JPanel rightRP = new JPanel();
		resultPanel.add( leftRP );
		resultPanel.add( rightRP );
		leftRP.setLayout( new GridLayout(7,1) );
		rightRP.setLayout( new GridLayout(7,1) );

		label = new JLabel("Unknown report format.");
		for ( int i = 0; i < all.length ; i++)
		    {
			String[] line = all[i].split(":");
			if (line[0].equals("Problem"))
			    {
				
				if (line[1].equals("SSGen"))
				    {
					problem = 1;
					label.setText("State Space Generation");
				    }
				if (line[1].equals("Safety"))
				    {
					problem = 2;
					label.setText("Verification of safety properties");
				    }
				if (line[1].equals("LTL MC"))
				    {
					problem = 3;
					label.setText("LTL Model Checking");
				    }
			    }

			if (line[0].equals("Workstations"))
			    {
				nodeNames = new TreeMap<String,String>();
				String[] names = line[1].split(" ");
				for (int j = 0; j < names.length; j++ )
				    {
					String[] name = names[j].split("\\.");
					nodeNames.put(new String().valueOf(j+2),name[0]);
					// j+2 because values start at position 2
				    }
			    }

			if (line[0].equals("Algorithm"))
			    {
				algorithmCommand = "divine."+line[1];
			    }

			if (line[0].equals("Nodes"))
			    {
				numberNodes = new Integer(line[1]).intValue();
			    }

			if (line[0].equals("ReporterCreated"))
			    {
				String tmp = line[1];
				for (int k=2; k<line.length; k++)
				    tmp = tmp + ":" + line[k];
				
				reportTimeAndDate = tmp;
			    }

			if (line[0].equals("CEGenerated"))
			    {
				if (line[1].equals("YES") || line[1].equals("Yes"))
				    {
					ce = true;
				    }
			    }
                        
                        if (line[0].equals("Time") && line[1].equals("max"))
                            {
                                runTime=line[2];
                            }  

                        if (line[0].equals("Trans") && line[1].equals("sum"))
                            {
                                transNumber=line[2];
                            }  

                        if (line[0].equals("CrossTrans") && line[1].equals("sum"))
                            {
                                crossNumber=line[2];
                            }  

                        if (line[0].equals("States") && line[1].equals("sum"))
                            {
                                statesNumber=line[2];
                            }  

		    }
		
		if (problem == 0)
		    {
			panel.setLayout(new java.awt.BorderLayout() );
			panel.add (label, java.awt.BorderLayout.CENTER );
			scrollPane.setViewportView( panel );
			return;
		    }

		panel.setLayout( new BoxLayout( panel, BoxLayout.Y_AXIS ) );
		
		leftRP.add(new JLabel("Verification problem:"));
		rightRP.add( label );

		switch (problem) {
		case 1:
		    break;
		case 2:
		    leftRP.add ( new JLabel("Verification result:") );
		    label = new JLabel("Safety properties are satisfied.");
		    label.setForeground(new Color(0x00ee00));
 		    for ( int i = 0; i < all.length ; i++)
			{
			    String[] line = all[i].split(":");
			    if (line[0].equals("SafetyViolated"))
				{				
				    if (line[1].equals("Yes") || line[1].equals("YES"))
					{
					    label = new JLabel("Safety properties are violated.");
					    label.setForeground(new Color(0xff0000));
					}
				}
			}
		    rightRP.add( label );
		    break;
		case 3:
		    leftRP.add( new JLabel("Verification result:") );
		    label = new JLabel("Invalid");
		    label.setForeground(new Color(0xff0000));
 		    for ( int i = 0; i < all.length ; i++)
			{
			    String[] line = all[i].split(":");
			    if (line[0].equals("IsValid"))
				{				
				    if (line[1].equals("YES") || line[1].equals("Yes"))
					{
					    label = new JLabel("Valid");
					    label.setForeground(new Color(0x00ee00));
					}
				}
			}
		    rightRP.add ( label );
		    break;
		}

		leftRP.add( new JLabel("Called algorithm:"));
		rightRP.add( new JLabel(algorithmCommand) );
		
		leftRP.add( new JLabel("Workstations participated:"));
		rightRP.add( new JLabel( new String().valueOf(numberNodes)) );
		
		leftRP.add( new JLabel("Counterexample generated:  "));
		rightRP.add( new JLabel((ce?"Yes":"No")) );

       		leftRP.add( new JLabel("Time of verification:  "));
		rightRP.add( new JLabel((runTime+" sec")) );

		leftRP.add( new JLabel("Report created:"));
		rightRP.add( new JLabel(reportTimeAndDate) );
		
		JPanel p = new JPanel();
		p.setLayout( new BorderLayout() );
		p.add ( resultPanel, BorderLayout.WEST );

 		panel.add ( p );
		panel.add ( new JLabel(" ") );
		
		resultPanel = new JPanel();
		leftRP = new JPanel();
		rightRP = new JPanel();
		resultPanel.add ( leftRP );
		resultPanel.add ( rightRP );
		leftRP.setLayout( new GridLayout(3,1) );
		rightRP.setLayout( new GridLayout(3,1) );
		leftRP.add( new JLabel("Total number of discovered states: ") );
		rightRP.add( new JLabel(statesNumber) );
		leftRP.add( new JLabel("Total number of discovered transitions: ") );
		rightRP.add( new JLabel(transNumber) );
		leftRP.add( new JLabel("Total number of cross transitions: ") );
		rightRP.add( new JLabel(crossNumber) );
		
		p = new JPanel();
		p.setLayout( new BorderLayout() );
		p.add ( resultPanel, BorderLayout.WEST );
 		panel.add ( p );
		panel.add ( new JLabel(" ") );

		resultPanel = new JPanel();
		int count=0;

		for ( int i = 0; i < all.length ; i++)
		    {
			if (all[i].contains("values"))
			    {
				count++;
			    }
		    }

		resultPanel.setLayout( new BoxLayout( resultPanel, BoxLayout.Y_AXIS ) );
		for ( int i = 0; i < all.length ; i++)
		    {
			if (all[i].contains("values"))
			    {
				String[] line = all[i].split(":");
				double c = 1;
				String text = "";
                                                                
				if (line[0].equals("Time"))
				{
                                    continue;
                                }
                                
                                if (line[0].equals("Time"))
				{                                        
				    int max = 0;
				    for (int j = 2; j < line.length; j++)
					{
					    if (new Float(line[j]).intValue() > max)
						max = new Float(line[j]).intValue();
					}
				    if (max == 0)
					{
					    c = 1000;
					    text = "RunTime (in miliseconds).";
					}
				    else
					{
					    text="RunTime (in seconds).";
					}
				}

				if (line[0].equals("VMSize"))
				{
				    text="Memory occupied (in MB).";
				    c = 1.0/1024;
				}

				if (line[0].equals("MsgSent"))
				{
				    text="Total number of messages sent by each workstation.";
				}

				if (line[0].equals("MsgSentUser"))
				{
				    text="Number of messages sent by underlying algorithm. (Excludes termination detection.)";
				}

				if (line[0].equals("StatesStored"))
				{
				    text="Terminal number of states stored on each workstation.";
				}

				if (line[0].equals("SuccsCalls"))
				{
				    text="Number of calls to the function for generating successors.";
				}

				if (line[0].equals("InitTime"))
				{
				    text="Time of network initialization (in miliseconds).";
				    c=1000;
				}

				if (line[0].equals("States"))
				{
				    text="States partitioned to individual workstations.";
				}

				if (line[0].equals("CrossTrans"))
				{
				    text="Cross-tranisitions starting at individual workstations.";
				}

				if (line[0].equals("Trans"))
				{
				    text="Transitions of states partitioned to individual workstations.";
				}

				DistrGraph dg = new DistrGraph((text.equals("") ? line[0] : text),
							       "");		

				for (int j = 2; j < line.length; j++)
				    {
					double d = c*new Float(line[j]).floatValue();
					dg.addNode(nodeNames.get(new String().valueOf(j)),
//  						   (j-2<10 ? "0" : "" )+
//  						   new String().valueOf(j-2),
						   new Float(d).intValue()
						   );
				    }
				//dg.hideMinmaxPanel();
				resultPanel.add ( dg ); 
				resultPanel.add ( new JPanel().add(new JLabel(" ")) );
			    }
		    }

		panel.add( resultPanel );

		panel.add ( new JLabel(" ") );
		scrollPane.setViewportView( panel );

	    }
    }	
}

 







