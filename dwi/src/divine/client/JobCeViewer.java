package divine.client;
import java.awt.Point;
import java.awt.event.*;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Component;
import java.awt.Panel;
import java.io.File;
import java.util.Set;
import java.util.HashSet;
import java.util.Vector;
import javax.swing.event.*;
import javax.swing.*;
import javax.swing.BoxLayout;
import divine.common.*;
import divine.common.Cluster.Executable.State;

public class JobCeViewer extends MainWindow.NodePanel
{
    private Panel m_codeContainer;
    private CeListRenderer m_cellRenderer;
    private Client m_client;
    private Vector<javax.swing.JScrollPane> m_codeScrolls;
    private javax.swing.JScrollPane m_watchesScroll;
    private javax.swing.JSplitPane m_codeWatchesSplitter;
    private Vector<javax.swing.JList> m_codes;
    private javax.swing.JList m_watches;
    private Vector<String> m_lastWatchesData;
    private String m_text;
    private CeBrowseToolBar m_toolbar;
    private Job m_currentJob;
    
    class CeListRenderer extends DefaultListCellRenderer
    {
        private Set m_toHighlight;
                
        public CeListRenderer()
        {
            m_toHighlight = new HashSet();
            setOpaque(true);
        }

        public void setToHighlight(int [] lines)
        {
            m_toHighlight.clear();
            for (int line : lines)
                m_toHighlight.add(line-1); //line >= 1 ... conversion to value from 0
        }

        public Component getListCellRendererComponent(
                                           JList list,
                                           Object value,
                                           int index,
                                           boolean isSelected,
                                           boolean cellHasFocus)
        {
            //Get the selected index. (The index param isn't
            //always valid, so just use the value.)
//            int selectedIndex = ((Integer)value).intValue();

//            super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);

            if (m_toHighlight.contains(index)) {
                setBackground(Color.ORANGE);
                setForeground(Color.BLACK);
            } else {
                setBackground(Color.WHITE);
                setForeground(Color.BLACK);
            }

            setText(value.toString());
            setFont(list.getFont());

            return this;
        }
    }
    
    public class CeBrowseToolBar extends javax.swing.JToolBar {
        private JobCeViewer ceViewer;

        private javax.swing.JTextField m_state;
        private javax.swing.JLabel m_maxState;
        private javax.swing.JLabel m_firstCycleState;
        private javax.swing.JLabel m_currentLines;
        
        private JButton buttonBack;
        private JButton buttonForward;
        private JButton buttonBegin;
        private JButton buttonEnd;
        private JButton buttonCycle;
        private JButton buttonWatch;
        
        public CeBrowseToolBar( JobCeViewer parentCeViewer )
        {
            ceViewer = parentCeViewer;
            
            m_state = new javax.swing.JTextField();
            m_state.setText("");
            m_state.setHorizontalAlignment(JTextField.TRAILING);
            m_state.setMaximumSize(new Dimension(m_state.getFontMetrics(m_state.getFont()).stringWidth("00000000"),
                                                 m_state.getFontMetrics(m_state.getFont()).getHeight()+6));
            m_state.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    javax.swing.JTextField textField = (JTextField)e.getSource();
                    try
                    {
                        int integerToSet = Integer.parseInt(textField.getText()) - 1;
                        ceViewer.goToState(integerToSet);
                    }
                    catch (NumberFormatException exception)
                    {
                        JOptionPane.showMessageDialog(ceViewer, "Invalid number format.",
                                                      "Warning:", JOptionPane.WARNING_MESSAGE);
                        ceViewer.update();
                    }
                }
            });
            
            m_maxState = new javax.swing.JLabel();
            m_maxState.setText("");
            
            m_firstCycleState = new javax.swing.JLabel();
            m_firstCycleState.setText("");
            
            buttonBack = createToolBarButton("ce_back.png", "CE_BACK", "One step back", "Back");
            buttonBack.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    ceViewer.previousState();
                }
            });
            
            buttonForward = createToolBarButton("ce_forw.png", "CE_FORWARD", "One step forward", "Forw");
            buttonForward.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    ceViewer.nextState();
                }
            });
            
            buttonBegin = createToolBarButton("ce_begin.png", "CE_BEGIN", "To begin", "Begin");
            buttonBegin.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    ceViewer.firstState();
                }
            });

            buttonEnd = createToolBarButton("ce_end.png", "CE_END", "To end", "End");
            buttonEnd.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    ceViewer.lastState();
                }
            });

            buttonCycle = createToolBarButton("ce_cycle.png", "CE_CYCLE", "To first state of cycle", "Cycle");
            buttonCycle.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    ceViewer.firstCycleState();
                }
            });

            buttonWatch = createToolBarButton("ce_watches.png", "CE_WATCH", "Add to watches", "Watches");
            buttonWatch.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    ceViewer.showDialogAddToWatches(ceViewer.getCurrentJob().getTraceInfo().getWatchCount());
                }
            });
            
            m_currentLines = new javax.swing.JLabel();
            m_currentLines.setText("");
            
            add(buttonBegin);
            add(buttonBack);
            addSeparator();
            add(m_state);
            add(m_maxState);
            addSeparator();
            add(buttonForward);
            add(buttonEnd);
            add(buttonCycle);
            addSeparator();
            add(m_firstCycleState);
            addSeparator();
            add(buttonWatch);
            addSeparator();
            add(m_currentLines);
        }
        
        public void setEnabled(boolean enabled)
        {
            if (enabled!=isEnabled())
            {
                super.setEnabled(enabled);
                for (Component component : getComponents())
                    component.setEnabled(enabled);
            }
        }
        
        public void setState(int state)
        {
            m_state.setText(Integer.toString(state));
        }
        
        public void setLines(int [] lines)
        {
            String text;
            if (lines.length==1) text = "Line: ";
            else text = "Lines: ";
                
            for (int i=0; i<lines.length; ++i)
            {
                text += lines[i];
                if (i!=lines.length-1) text += " & ";
            }
            
            m_currentLines.setText(text);
        }
        
        public void setMaxState(int maxState)
        {
            m_maxState.setText(" / "+Integer.toString(maxState));
        }
        
        public void setFirstCycleState(int firstCycleState)
        {
            if (firstCycleState>0) m_firstCycleState.setText("Cycle at state " +
                                           Integer.toString(firstCycleState));
            else
            {
                m_firstCycleState.setText("No cycle present.");
                buttonCycle.setEnabled(false);
            }
        }
        
        protected JButton createToolBarButton(String imageName,
                                       String actionCommand,
                                       String toolTipText,
                                       String altText)
        {
            //Look for the image.
            String imgLocation = 
                new File( ceViewer.getClient().imageDir(), imageName ).getAbsolutePath();
            //Create and initialize the button.
            JButton button = new JButton();
            button.setActionCommand(actionCommand);
            button.setToolTipText(toolTipText);

            if (imgLocation != null) {                      //image found
                button.setIcon(new ImageIcon(imgLocation));
            } else {                                     //no image found
                button.setText(altText);
                System.err.println("Resource not found: " + imgLocation);
            }

            return button;
        }
        
    }
    
    public JobCeViewer( Client client, Job job )
    {
        m_client = client;
        m_codeContainer = new Panel();
        m_codeContainer.setLayout(new javax.swing.BoxLayout(m_codeContainer,javax.swing.BoxLayout.Y_AXIS));
        m_codeScrolls = new Vector<javax.swing.JScrollPane>();//javax.swing.JScrollPane();
        m_watchesScroll = new javax.swing.JScrollPane();
        m_cellRenderer = new CeListRenderer();
        m_codes = new Vector<javax.swing.JList>();//javax.swing.JList();
        m_watches = new javax.swing.JList();
        m_watches.setVisibleRowCount(4);
        m_lastWatchesData = new Vector<String>();
        m_text = "";
        
        setLayout( new java.awt.BorderLayout() );
        m_toolbar = new CeBrowseToolBar(this);
        

        m_currentJob = null;
                
//        m_code.setCellRenderer(m_cellRenderer);
//        m_codeScroll.setViewportView( m_code );
        m_watchesScroll.setViewportView( m_watches );

//        m_codeWatchesSplitter = new javax.swing.JSplitPane(JSplitPane.VERTICAL_SPLIT,
//                           m_codeScroll, m_watchesScroll);
        m_codeWatchesSplitter = new javax.swing.JSplitPane(JSplitPane.VERTICAL_SPLIT,
                           m_codeContainer, m_watchesScroll);
        m_codeWatchesSplitter.setOneTouchExpandable(true);
        m_codeWatchesSplitter.setResizeWeight(0.8);

        add( m_codeWatchesSplitter, java.awt.BorderLayout.CENTER );
        add( m_toolbar, java.awt.BorderLayout.PAGE_START );
           
        update( (Node) job );
    }

    public Client getClient()
    {
        return m_client;
    }
    
    protected boolean update() {
        return update( m_currentJob );
    }
    
    public boolean update( Node e ) {
        if ( ! (e instanceof Job) ) 
            return false;
        Job job = ( Job ) e;
        return update( job );
    }

    protected boolean update( Job job ) {
        boolean forceUpdate = (job!=m_currentJob);
        m_currentJob = job;

        // avoid handling broken jobs
        if ( job.task() == null || job.task().algorithm() == null
             || job.task().property() == null || job.cluster() == null ) return false;

        // here update the view to reflect the data in Job j
        int [][] traceLines = job.ceTraceLines();
        
        
//        setText(job.ceTrace());
        int currentState = m_currentJob.getTraceInfo().getCurrentState();
        
        if (job.ceExists() && traceLines != null && currentState>=0)
        {
            int neededViewportCount = traceLines[currentState].length;
            setText( job.task().model().text(), neededViewportCount, forceUpdate );
            for (int i=0; i<neededViewportCount; ++i)
                m_codeScrolls.get(i).setVisible(true);
            for (int i=neededViewportCount; i<m_codeScrolls.size(); ++i)
                m_codeScrolls.get(i).setVisible(false);
            m_codeContainer.validate();
            m_cellRenderer.setToHighlight(traceLines[currentState]);
            m_toolbar.setEnabled(true);
            m_toolbar.setState(currentState+1);
            m_toolbar.setLines(traceLines[currentState]);
            m_toolbar.setMaxState(m_currentJob.getTraceInfo().getMaxState()+1);
            m_toolbar.setFirstCycleState(m_currentJob.getTraceInfo().getFirstCycleState()+1);
            Vector<String> watchesData = new Vector<String>();
            for (Integer varIndex : job.getTraceInfo().getWatchList())
            {
                String varName = job.ceVariableValues().getName(varIndex);
                String varValue = job.ceVariableValues().getValue(currentState,varIndex);
                watchesData.add(varName + ": " + varValue);
            }
            
            if (!m_lastWatchesData.equals(watchesData))
            {
                m_lastWatchesData = watchesData;
                m_watches.setListData(watchesData);
                m_codeWatchesSplitter.validate();
                m_watches.repaint();
            }
            
            
            m_codeContainer.repaint();
            if (traceLines[currentState].length>0)
                for (int i=0; i<neededViewportCount; ++i)
                    m_codes.get(i).ensureIndexIsVisible(traceLines[currentState][i]-1); //line numbers >= 1 ... conversion to value from 0
        }
        else
        {
            if (job.ceExists())
                if (job.state() == State.Finished)
                    setText("Error: Invalid counterexample format",1);
                else
                    setText("Counterexample not yet available",1);
            else
                for (JScrollPane codeScroll : m_codeScrolls)
                    codeScroll.setVisible(false);
            m_toolbar.setEnabled(false);
            m_lastWatchesData = new Vector<String>();
            m_watches.setListData(m_lastWatchesData);
        }
        
        return true;
    }

    public void setText( String s, int viewportCount )
    {
        setText(s, viewportCount, false);
    }
    
    public void setText( String s, int viewportCount, boolean forceUpdate ) {
        if ( s == null ) return;
        
        if (viewportCount > getMaxViewportCount())
        {
            int origSize = m_codeScrolls.size();
            m_codeScrolls.setSize(viewportCount);
            m_codes.setSize(viewportCount);
            
            String [] splitted_text = m_text.split( "\n" );
            
            for (int i=origSize; i<m_codeScrolls.size(); ++i)
            {
                JList newCode = new JList();
                newCode.setFont(this.getFont());
                newCode.setListData(splitted_text);
                newCode.setCellRenderer(m_cellRenderer);
                JScrollPane newScrollPane = new JScrollPane();
                newScrollPane.setViewportView(newCode);
                m_codeScrolls.set(i,newScrollPane);
                m_codes.set(i,newCode);
                m_codeContainer.add(newScrollPane);
            }
        }
        
        if ( s.equals( m_text ) )
            return;
        m_text = s;
        
        String [] splitted_text = m_text.split( "\n" );
        
        for (int i=0; i<viewportCount; ++i)
            m_codes.get(i).setListData(splitted_text);
    }
    
    public int getMaxViewportCount()
    { return m_codeScrolls.size(); }

    public String text() { return m_text; }
    
    public void goToState(int stateIndex)
    {
        m_currentJob.getTraceInfo().setCurrentState(stateIndex);
//        JOptionPane.showMessageDialog(null,"Potvrzuji: " + Integer.toString(stateIndex) + " from: " + m_currentJob.toString());
        update();
    }
    
    public void nextState()
    {
        goToState(m_currentJob.getTraceInfo().getCurrentState()+1);
    }

    public void previousState()
    {
        goToState(m_currentJob.getTraceInfo().getCurrentState()-1);
    }

    public void firstState()
    {
        goToState(0);
    }

    public void lastState()
    {
        goToState(m_currentJob.getTraceInfo().getMaxState());
    }

    public void firstCycleState()
    {
        goToState(m_currentJob.getTraceInfo().getFirstCycleState());
    }
    
    public Job getCurrentJob()
    {
        return m_currentJob;
    }
    
    public void showDialogAddToWatches(int newWatchPosition)
    {
        String[] possibleValues = m_currentJob.ceVariableValues().getNames();

        String chosen = (String)JOptionPane.showInputDialog(null,
            "Choose a variable to watch:", "Add watch...",
            JOptionPane.INFORMATION_MESSAGE, null,
            possibleValues, possibleValues[0]);
        
        if (chosen!=null)
        {
            int varIndex = m_currentJob.ceVariableValues().getVarIndex(chosen);
            m_currentJob.getTraceInfo().addWatch(newWatchPosition, varIndex);
            update();
        }
    }

}


