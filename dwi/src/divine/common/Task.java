// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;

import java.io.*;
import java.util.*;

public class Task extends Node
    implements Wired.Interface, Cloneable
{
    protected String m_model;
    protected String m_property;
    protected MapNode< Job > m_jobs;

    public MapNode< Job > jobs() { return m_jobs; }
   
    public Object clone() {
        Task t = (Task) super.clone();
        t.m_jobs = (MapNode) m_jobs.clone();
        t.m_jobs.setParentInternal( t );
        return t;
    }
    
    protected String m_algorithm;

    public boolean editable() {
        for ( Job j : jobs().values() )
            if ( !j.editable() )
                return false;
        return true;
    }

    public Model model() {
        if ( user() == null ) return null;
        Model m = user().models().get( m_model );
        if ( m == null )
            for ( Model n : user().models().values() )
                if ( n.hasProperty() ) {
                    m = n;
                    break;
                }
        return m;
    }

    public Property property() {
        if ( model() == null ) return null;
        return model().properties().getSome( m_property );
    }

    public boolean orphaned() {
        if ( user() == null ) return true;
        Model m = user().models().get( m_model );
        if ( m == null || m.deleted() )
            return true;
        if ( m.properties().get( m_property ) == null
                || m.properties().get( m_property ).deleted() ) return true;
        return false;
    }

    public Algorithm algorithm() {
        if ( user() == null ) return null;
        Algorithm a = user().algorithms().get( m_algorithm );
        if ( a == null )
            for ( Algorithm b : user().algorithms().values() )
                if ( b.canHandle( property() ) ) {
                    a = b;
                    break;
                }
        return a;
    }

    public void setAlgorithm( Algorithm a ) { m_algorithm = a.name(); setDirty(); }
    public void setAlgorithm( String s ) { m_algorithm = s; setDirty(); }

    public Object fromWire( String s )
        throws Exception
    {
        Map m = wireRead(
             s,
             "jobs", new MapNode( this, "jobs", new Job() ),
             "model", "",
             "property", "",
             "algorithm", "" );

        m_model = (String) m.get( "model" );
        m_property = (String) m.get( "property" );
        m_algorithm = (String) m.get( "algorithm" );
        m_jobs = (MapNode) m.get( "jobs" );
        return this;
    }
    
    public String toWire() // to server
        throws Exception
    {
        String algname = "";
        if ( algorithm() != null ) algname = algorithm().name();
        return wireWrite(
                "jobs", jobs(),
                "algorithm", algname,
                "model", m_model,
                "property", m_property );
    }

    public Map< String, Node > children() {
        return nodeMap( jobs() );
    }

    public Task()
    {
        super( null, "Task" );
        m_jobs = new MapNode( this, "jobs", new Job() );
        m_model = "";
        m_property = "";
        m_algorithm = "";
    }

    public void addJob( Job j ) {
        m_jobs.addUnique( j );
    }

    TreeMap< String, Object > runInput() 
    {
        return Wired.map( 
                "property-type", property().typeString(),
                "model-type", property().model().typeString(),
                "property", property().text(),
                "model", property().model().text() );
    }

    public String canonicalName() {
        String n = "no model";
        if ( model() != null )
            n = model().name();
        if ( property() != null )
            n = n + " - " + property().name();
        return n;
    }

    public void setProperty( Property p ) {
        if ( p == null ) {
            m_property = m_model = null;
        } else {
            m_model = p.model().name();
            m_property = p.name();
        }
        setDirty();
    }

    public User user() { return (User) parent().parent(); }
}
