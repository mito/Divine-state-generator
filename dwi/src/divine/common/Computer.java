package divine.common;
import java.util.*;

public class Computer extends Node implements Wired.Interface, Cloneable
{
    protected int m_cpuCount;
    protected float m_load;
    protected Cluster m_cluster;

    public float load() { return m_load; }
    public void setLoad( String s ) {
        String near = s.split( "," )[0];
        if ( near.equals( "" ) )
            m_load = -1;
        else
            m_load = Float.parseFloat( near );
        // Logger.log( 4, m_name + " load = " + m_load );
    };

    public Computer()
    {
        this.m_load = 0;
        this.m_cpuCount = 1; // default
    }

    public int cpuCount() { return m_cpuCount; }

    public Cluster cluster() { return (Cluster) parent().parent(); }

    protected boolean m_available;
    public boolean isAvailable() { return m_available; }

    public Object clone() {
        return super.clone(); }

    public Object fromWire( String s )
        throws Exception
    {
        Map< String, Object > m = (Map) Wired.from(
                Wired.map( "load", "", "cpu-count", "" ), s );
        m_load = Float.parseFloat( (String) m.get( "load" ) );
        m_cpuCount = Integer.parseInt( (String) m.get( "cpu-count" ) );
        return this;
    }

    public String toWire()
        throws Exception
    {
        return Wired.to(
                Wired.map(
                        "load", String.valueOf( m_load ),
                        "cpu-count", String.valueOf( m_cpuCount ) ) );
    }
}
