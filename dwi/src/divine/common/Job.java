// -*- mode: java; c-basic-offset: 4 -*- 

package divine.common;

import java.io.*;
import java.util.*;
import java.util.List;
import java.util.LinkedList;
import divine.common.Cluster.Executable.State;
import javax.swing.JOptionPane;

public class Job extends Cluster.Executable
    implements Wired.Interface, Cloneable
{
    protected static final String NO_COUNTEREXAMPLE = "### NO COUNTEREXAMPLE ###";
    
    protected String m_cluster;
    protected String m_output;
    protected String m_stdOut, m_lastLog, m_report, m_ceTrace, m_ceStates;
    protected int m_walltime; // in minutes
    protected int [][] m_ceTraceLines;
    protected int m_cycleIndex;
    protected CeStateParseResults m_ceVariableValues;
    protected boolean m_ceExists;
    protected int m_machines;
    protected TraceInfo m_traceInfo;

    public class VariableValue {
        public VariableValue(String argVariable, String argValue)
        { variable = argVariable; value = argValue; }
        public String variable;
        public String value;
    }
    
    public class CeParseResults {
        public static final int NO_CYCLE = -1;
        public int [][] traceLines;
        public int cycleIndex;
    }
    
    public class CeStateParseResults {
        
        private String [][] values;
        private String [] names;
        
        public CeStateParseResults() { values = null; names = null; }
        
        public String getValue(int stateIndex, int varIndex)
        {
            if (stateIndex<0 || stateIndex>=values.length)
                return "<Invalid state index>";
            else
                if (varIndex<0 || varIndex>=values[stateIndex].length)
                    return "<Unknown identifier>";
                else
                    return values[stateIndex][varIndex];
        }
        public String [] getNames() { return names; }
        
        public int getVarIndex(String varName)
        {
            boolean found = false;
            int varIndex = 0;
            for (int i=0; i!=names.length && found == false; ++i)
                if (names[i].equals(varName)) { varIndex = i; found = true; }
            if (found) return varIndex;
            else return -1;
        }
        
        public String getName(int varIndex)
        {
            return names[varIndex];
        }
    
        public void setNames(String [] newNames)
        {
            names = newNames;
            if (values!=null)
                for (int i=0; i!=values.length; ++i) values[i] = new String[names.length];
        }
        
        public void setStateCount(int stateCount)
        { 
            values = new String[stateCount][];
            if (names!=null)
                for (int i=0; i!=values.length; ++i) values[i] = new String[names.length];
        }
          
        public void setValue(int stateIndex, int varIndex, String value)
        { values[stateIndex][varIndex] = value; }
    }
    
    public Job() {
        super( null, "Job" );
        m_machines = 1;
        m_report = m_cluster = m_output = m_stdOut = m_lastLog = m_ceTrace = m_ceStates = "";
        m_ceTraceLines = null;
        m_ceExists = false;
        m_traceInfo = new TraceInfo();
        m_walltime = 60;
    }

    public Object clone()
    {
        Job result = (Job)super.clone();
        result.m_traceInfo = (TraceInfo)m_traceInfo.clone();
        return result;
    }
    
    public User user() { return task().user(); }
    public Task task() { return (Task) parent().parent(); }
    public Property property() { return task().property(); }
    public Model model() { return task().model(); }
    public boolean editable() { return mutable(); }

    public void setMachines( Integer m ) { m_machines = m; setDirty(); }
    public int machines() { return m_machines; }
    
    public String output( String w )
        throws Exception
    {
        if ( w == null ) return m_output;
        if ( m_output == null ) return null;
        return (String) ( (Map) Wired.from( Wired.mapTemplate( "" ),
                        m_output ) ).get( w );
    }

    public String generateLastLog()
        throws Exception
    {
        StringBuffer log = new StringBuffer();
        String l = output( "algorithm.00" );
        if ( l != null ) {
            String[] spl = l.split( "\n" );
            log.append( spl[1] + "\n" + spl[2] + "\n" + spl[3] + "\n" );
            for( int i = 0; i < 99; ++ i ) {
                String s = output( String.format( "algorithm.%02d", i ) );
                if ( s != null ) {
                    spl = s.split( "\n" );
                    int line = spl.length - 1;
                    if ( line > 2 && spl[ line ].charAt( 0 ) == '#' )
                        line --;
                    log.append( // String.format( "%02d: ", i ) +
                            spl[ line ] + "\n" );
                } else
                    break;
            }
        }
        return log.toString();
    }
    
    public String lastLog() { return m_lastLog; }
    public String stdOut() { return m_stdOut; }
    public String report() { return m_report; }

    public boolean ceExists() { return m_ceExists; }
    public int [][] ceTraceLines() { return m_ceTraceLines; }
    public String ceTrace() { return m_ceTrace; }
    public String ceStates() { return m_ceStates; }
    
    public CeParseResults parseCeTrace( String trace )
    {
        CeParseResults result = new CeParseResults();
        boolean invalid = false;
        //splits traceText to an array of lines
        String [] linesOfTraceText = trace.split("\n");
        String [] splittedLine;
        int foundCycle = 0;
        result.traceLines = new int[linesOfTraceText.length][];
        result.cycleIndex = CeParseResults.NO_CYCLE;
        //converts each line (containing sequence of integers) to sequence
        //of integers
        for (int i=0; i<linesOfTraceText.length && !invalid; ++i)
        {
            splittedLine = linesOfTraceText[i].split(" ");
            if (splittedLine.length>0 && splittedLine[0].equals("CYCLE"))
            {
                result.cycleIndex = i;
                //reallocation of array (decreasing its length by 1)
                int [][] auxTraceLines = new int[result.traceLines.length-1][];
                for (int j=0; j<i; ++j)
                    auxTraceLines[j] = result.traceLines[j];
                result.traceLines = auxTraceLines;
                //cycle was found - index to the resulting array should be decreased by 1
                foundCycle++;
            }
            else
            {
                result.traceLines[i-foundCycle] = new int[splittedLine.length];
                for (int j=0; j<splittedLine.length && !invalid; ++j)
                    try
                    { result.traceLines[i-foundCycle][j] = Integer.parseInt(splittedLine[j]); }
                    catch (NumberFormatException e)
                    { invalid = true;}
            }
        }
        
        if (invalid) return null;
        else return result;
    }

    public CeStateParseResults parseCeStates( String states )
    {
        CeStateParseResults result = new CeStateParseResults();
        boolean invalid = false;
        List< List<VariableValue> > varValuesInStates = new LinkedList< List<VariableValue> >();
        
        //splits traceText to an array of lines
        String [] linesOfStates = states.split("\n");
        boolean isFirst = true;
        List<String> nameList = new LinkedList<String>();
        String [] names = null;
        
        for (String line : linesOfStates)
        {
            if (!line.matches(" *CYCLE *"))
            {
                String [] splittedLine = line.split(",");
                List<VariableValue> varValues = new LinkedList<VariableValue>();
                int pairIndex = 0;
                for (String pair : splittedLine)
                {
                    String [] splittedPair = pair.split(":");
                    if (splittedPair.length == 2)
                    {
                        varValues.add(new VariableValue(splittedPair[0],splittedPair[1]));
                        if (isFirst)
                        {
                            //stores variable names from the first state (then copied to array `names')
                            nameList.add(splittedPair[0]);
                        }
                        else
                        {
                            //controls, that all states have same var. names
                            if (!splittedPair[0].equals(names[pairIndex]))
                                invalid = true;
                            pairIndex++;
                        }
                            
                    }
                    else invalid = true;
                }
                varValuesInStates.add(varValues);
                if (isFirst)
                {
                    isFirst = false;
                    names = new String[nameList.size()];
                    nameList.toArray(names);
                }
            }
        }
        
        if (names==null) invalid = true;
        if (varValuesInStates.size()==0) invalid = true;
        
        if (invalid) return null;
        else
        {
            result.setStateCount(varValuesInStates.size());
                     
            result.setNames(names);
                    
            int stateIndex = 0;
            for (List<VariableValue> varValuesInOneState : varValuesInStates)
            {
                int varIndex = 0;
                for (VariableValue varValue : varValuesInOneState)
                {
                    result.setValue(stateIndex, varIndex, varValue.value);
                    varIndex++;
                }
                stateIndex++;
            }
            
            return result;
        }
         
    }
    
    public boolean checkCeExistence( String trace )
    {
        if (trace.equals(NO_COUNTEREXAMPLE))
            return false;   //counterexample doesn't exist
        else
            return true;
    }
    
    public Cluster cluster() {
        if ( user() == null ) return null;
        return user().clusters().getSome( m_cluster );
    }

    public void setCluster( String s ) {
        m_cluster = s; setDirty();
        if ( cluster() != null && m_machines > cluster().computerCount() )
            m_machines = cluster().computerCount();
    }

    public void setCluster( Cluster c ) {
        if ( c == null )
            return;
        setCluster( c.name() );
    }
    
    class JobKiller extends Cluster.Executable {
        public void data( String d ) {}
        public void run( Cluster c ) {
            int tries = 3;
            int ok = 1;
            while ( tries > 0 && ok != 0 ) {
                try {
                    ok = execute( c, Wired.to( Wired.map( "command", "qdel "
                                            + output( "pbs-job-id" ) ) ) );
                } catch( Exception e ) {
                    Logger.log( e );
                }
                tries --;
                try {
                    if ( tries > 0 && ok != 0 )
                        Thread.sleep( 10000 ); // try again later, maybe it'll work?
                } catch( Exception e ) { Logger.log( e ); }
            }
        }
    }

    public void run( Cluster c )
    {
        try {
            StringBuffer b = new StringBuffer();
            b.append( "divine.dwi-prepare --mode=" + c.typeString()
                    + " --exec=" + task().algorithm().executable()
                    + " --workdir=. --args=arguments --input=input --output=script"
                    + " > prepare-stdout 2> prepare-stderr\n" );
            b.append( "./script\n" );
    
            Integer r = 13;
            try {
                r = execute( c, Wired.to(
                                Wired.map(
                                        "command", b.toString(),
                                        "arguments", runArgs(),
                                        "input", task().runInput() ) ) );
            } catch ( Exception e ) { Logger.log( e ); }

            if ( r > 0 ) {
                m_output = m_output + "algorithm-stdout: Process died "
                    + "unexpectedly with exit code " + r.toString() + "\n";
                m_state = State.Aborted;
                notifyOutputChanged();
            } else
                m_state = State.Finished;

        } catch ( Exception e ) {
            m_abort = true;
            Logger.log( e );
        }

        if ( m_abort ) {
            m_output = m_output + "algorithm-stdout: Aborted\n";
            notifyOutputChanged();
            m_state = State.Aborted;
            // we run this inline, not starting a new thread
            if ( c.type() == Cluster.Type.PBS )
                cluster().runner( new JobKiller() ).run();
        }

        setDirty();

    }

    public void data( String s ) {
        m_output = s;
        notifyOutputChanged();
    }

    public void notifyOutputChanged() {
        try {
            m_lastLog = generateLastLog();
            m_stdOut = output( "algorithm-stdout" );
            m_report = output( "algorithm.report" );
            m_ceTrace = output( "algorithm.cetrace" );
            m_ceStates = output( "algorithm.cestates" );
            if ( output( "pbs-job-id" ) != null )
                m_state = State.Queued;
            if ( output( "started" ) != null )
                m_state = State.Running;
            if ( output( "finished" ) != null )
                m_state = State.Finished;
        } catch ( Exception e ) { Logger.log( e ); }
        setDirty();
    }

    public void setWalltime( int minutes ) { m_walltime = minutes; setDirty(); }
    public int walltime() { return m_walltime; }
    public String walltimeFormatted() {
        int hrs = walltime() / 60;
        int mins = walltime() % 60;
        StringBuffer b = new StringBuffer();
        if ( hrs > 0 ) {
            b.append( hrs );
            b.append( ":" );
        }
        b.append( mins );
        b.append( ":00" );
        return b.toString();
    }
    
    public TreeMap runArgs() {
        TreeMap argm = new TreeMap(); // task().arguments() );
        argm.put( "machines", m_machines == 0 ?
                cluster().computerCount() : m_machines );
        List< Computer > boxen = cluster().leastLoaded( m_machines );
        StringBuffer mfb = new StringBuffer();
        for ( Computer com : boxen )
            mfb.append( com.name() + "\n" );
        argm.put( "mf", mfb.toString() );
        argm.put( "walltime", walltimeFormatted() );
        argm.put( "jobname", task().name()+"("+machines()+")" );
        return argm;
    }

    public Object fromWire( String s )
        throws Exception
    {
        Map m = wireRead( s,
                          "state", "",
                          "walltime", "",
                          "o", "",
                          "l", "",
                          "so", "",
                          "cetrace", "",
                          "cestates", "",
                          "report", "",
                          "cluster", "",
                          "machines", "" );

        m_state = stateFromString( (String) m.get( "state" ) );
        m_output = (String) m.get( "o" );
        m_stdOut = (String) m.get( "so" );
        m_lastLog = (String) m.get( "l" );
        m_cluster = (String) m.get( "cluster" );
        m_ceTrace = (String) m.get( "cetrace" );
        m_ceStates = (String) m.get( "cestates" );
        m_ceExists = checkCeExistence(m_ceTrace);

        if (ceExists())
        {
            if (m_ceTraceLines==null) //once it it created, it is never rewritten
            {
                CeParseResults auxParseResults = parseCeTrace(m_ceTrace);
                if (auxParseResults!=null)
                {
                    m_ceTraceLines = auxParseResults.traceLines;
                    m_cycleIndex = auxParseResults.cycleIndex;
                }
                else
                {
                    m_ceTraceLines = null;
                    m_cycleIndex = CeParseResults.NO_CYCLE;
                }
                
                if (m_ceTraceLines!=null)
                {
                    m_traceInfo.setMaxState(m_ceTraceLines.length-1);
                    m_traceInfo.setCurrentState(0);
                    m_traceInfo.setFirstCycleState(m_cycleIndex);
                }
                else m_traceInfo.setMaxState(-1);
                
                m_ceVariableValues = parseCeStates(m_ceStates);
                //m_ceVariableValues = null;
                if (m_ceVariableValues==null)
                {//invalid CE => reset also trace
                    m_ceTraceLines = null;
                    m_cycleIndex = CeParseResults.NO_CYCLE;
                }
            }
        } else {
            m_traceInfo.setMaxState(-1);
        }

        m_report = (String) m.get( "report" );
        m_machines = readInteger( 1, m, "machines" );
        m_walltime = readInteger( 60, m, "walltime" );
        return this;
    }
    
    public String toWire() // to server
        throws Exception
    {
        return wireWrite(
                "state", stateString(),
                // "o", m_output,
                "walltime", m_walltime,
                "l", m_lastLog,
                "report", m_report,
                "so", m_stdOut,
                "cetrace", m_ceTrace,
                "cestates", m_ceStates,
                "cluster", m_cluster,
                "machines", m_machines );
    }

    public boolean running() {
        return state() == State.Running;
    }

    public void execute() throws Exception {
        Cluster c = cluster();
        if ( user() == null )
            throw new Exception( "Trying to execute a job without owner" );
        if ( c == null )
            throw new Exception( "No cluster available" );
        Thread t = new Thread( cluster().runner( this ) );
        t.start();
    }

    public String canonicalName() {
        String n = "no cluster", m;
        if ( cluster() != null )
            n = cluster().name();
        if ( machines() == 1 )
            m = machines() + " node";
        else
            m = machines() + " nodes";
        n = n + " - " + m;
        return n;
    }

    
    public TraceInfo getTraceInfo() { return m_traceInfo; }
    
    public CeStateParseResults ceVariableValues() { return m_ceVariableValues; }
    
    public class TraceInfo implements Cloneable
    {
        private int m_currentState;
        private int m_maxState;
        private int m_firstCycleState;
        private List<Integer> m_watchList;
        
        public Object clone()
        // throws CloneNotSupportedException
        {
            try {
                return super.clone();
            } catch ( CloneNotSupportedException x ) {
                return null;
            }
        }

        public TraceInfo()
        {
            m_currentState = m_maxState = m_firstCycleState = -1;
            m_watchList = new LinkedList<Integer>();
        }
        
        public void setCurrentState(int newCurrentState)
        {
            if (newCurrentState>=0 && newCurrentState<=m_maxState)
                m_currentState = newCurrentState;
        }
        
        public int getCurrentState()
        { return m_currentState; }
        
        public void setFirstCycleState(int firstCycleState)
        {
            if (firstCycleState>=0 && firstCycleState<=m_maxState)
                m_firstCycleState = firstCycleState;
        }
        
        public int getFirstCycleState()
        { return m_firstCycleState; }
        
        public void setMaxState(int newMaxState)
        {
            m_maxState = newMaxState;
            if (m_currentState>m_maxState) m_currentState = newMaxState;
        }
        
        public int getMaxState()
        { return m_maxState; }
        
        public void addWatch(int wantedPosition, int varIndex)
        {
            if (wantedPosition>m_watchList.size())
                wantedPosition = m_watchList.size();
            if (wantedPosition<0)
                wantedPosition = 0;
            m_watchList.add(wantedPosition,varIndex);
        }
        
        public void removeWatch(int removedPosition)
        {
            if (removedPosition<m_watchList.size()&&
                removedPosition>=0)
                m_watchList.remove(removedPosition);
        }
        
        public List<Integer> getWatchList()
        { return m_watchList; }
        
        public int getWatchCount()
        { return m_watchList.size(); }
    }
    
}
