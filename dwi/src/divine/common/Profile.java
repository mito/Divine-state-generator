// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;
import java.util.Map;

public class Profile extends Node
    implements Wired.Interface, Cloneable
{
    protected String m_cluster;

    public Profile()
    {
        m_name = "Profile";
    }

    public User user() { return (User) parent(); }
    public Cluster cluster() { return user().clusters().get( m_cluster ); }

    public Object fromWire( String s )
        throws Exception
    {
        Map m = wireRead( s, "cluster", ""  );
        m_cluster = (String) m.get( "cluster" );
        return this;
    }

    public String toWire()
        throws Exception
    {
        return wireWrite(
                "cluster", m_cluster );
    }

    public Object clone()
    {
        return super.clone();
    }

}

