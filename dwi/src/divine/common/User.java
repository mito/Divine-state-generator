// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;
import java.lang.*;
import java.util.*;

public class User extends Node
    implements Wired.Interface, Cloneable
{
    protected MapNode< Model > m_models;
    protected MapNode< Cluster > m_clusters;
    protected MapNode< Algorithm > m_algorithms;
    protected MapNode< Task > m_tasks;
    protected MapNode< Profile > m_profiles;
    protected String m_passwordHash;
    protected boolean m_admin;
    protected boolean m_active;
    protected boolean m_loaded;

    public static class Passwd extends Node implements Wired.Interface
    {
        protected User m_user;
        public User user() { return m_user; }
        public String toWire() { return "wee"; }

        public Passwd() {
            m_user = new User();
        }

        public Object fromWire( String s )
            throws Exception
        {
            // m_user.setParentInternal( this );
            Map m = (Map) Wired.from(
                    Wired.map( "passwordHash", "", "admin", "" ), s );
            m_user.m_passwordHash = (String) m.get( "passwordHash" );
            m_user.m_admin = false;
            if (((String) m.get( "passwordHash" )).equals( "yes" ) )
                m_user.m_admin = true;
            return this;
        }

        public void setNameInternal( String n ) {
            super.setNameInternal( n );
            Logger.log( 4, "Passwd.setNameInternal" );
            m_user.setNameInternal( n );
        }

        public Object clone() {
            return super.clone();
        }
    }

    public Object clone() { return super.clone(); }

    public User self() { return this; }
    public boolean active() { return m_active; }
    public MapNode< Cluster > clusters() { return m_clusters; }
    public MapNode< Algorithm > algorithms() { return m_algorithms; }

    public void setClusters( MapNode< Cluster > c ) {
        m_clusters = c;
        c.setParentInternal( this );
    }

    public void setAlgorithms( MapNode< Algorithm > c ) {
        m_algorithms = c;
        c.setParentInternal( this );
    }

    public String userData()
        throws Exception
    {
        return Wired.to(
                Wired.map(
                        "m", models(),
                        "t", tasks(),
                        "p", profiles()
                    ) );
    }

    public void loadUserData( String s )
        throws Exception
    {
        Logger.log( 2, "user " + name() + ": loadUserData, loaded = " + m_loaded );
        if ( m_loaded )
            return;

        Map m = (Map) Wired.from(
                Wired.map(
                        "m", Wired.mapTemplate( new Model() ),
                        "p", Wired.mapTemplate( new Profile() ),
                        "t", Wired.mapTemplate( new Task() ) ), s );

        m_models.replace( (Map) m.get( "m" ) );
        m_tasks.replace( (Map) m.get( "t" ) );
        m_profiles.replace( (Map) m.get( "p" ) );
        activateModels();
        m_loaded = true;
    }

    public void activateModels() {
       /* for ( Model m : m_models.values() ) {
            m.setAvailableResources( clusters(), algorithms() );
            } */
    }

    public void deleteOrphanedTasks() {
        for ( Task t : m_tasks.values() ) {
            if ( t.orphaned() )
                t.setDeleted();
        }
    }

    public Map< String, Node > children() {
        return nodeMap( models(), algorithms(), clusters(), profiles(), tasks() );
    }

    public Collection< Task > tasksFlattened() {
        return m_tasks.values();
    }

    public MapNode< Task > tasks() {
        return m_tasks;
    }

    public MapNode< Profile > profiles() {
        return m_profiles;
    }

    /* public Task task( Node.Path path ) {
        Node.Path p = new Node.Path();
        p.putAll( (Map) path );
        return (Task) find( p );
        } */

    public void setActiveInternal( boolean active ) {
        Logger.log( 2, "user " + name() + ": setActiveInternal: " + active );
        m_active = active;
        boolean fl = true;
        if ( !active ) {
            for ( Task t : tasks().values() ) {
               for ( Job j : t.jobs().values() )
                  if ( j.running() )
                     fl = false;
            }
            if ( fl )
                flush();
        }
    }

    public void flush() {
        Logger.log( 2, "user " + name() + ": flush()" );
        m_loaded = false;
        models().clear();
    }

    public MapNode< Model > models() {
        return m_models;
    }

    public void addModel( Model m )
    {
        // m.setUserInternal( this );
        m_models.addUnique( m );
    }

    public void rename() {} // XXX throw?

    public boolean passwordOk( String password ) {
        return Global.createMD5( password ).equals( passwordHash() );
    }

    public String passwordHash()
    {
        return m_passwordHash;
    }

    public User()
    {
        m_models = new MapNode( this, "models", new Model() );
        m_clusters = new MapNode( this, "clusters", new Cluster() );
        m_algorithms = new MapNode( this, "algorithms", new Algorithm() );
        m_profiles = new MapNode( this, "profile", new Profile() );
        m_tasks = new MapNode( this, "tasks", new Task() );
        m_name = "user";
        m_passwordHash = "";
        m_active = false;
        m_loaded = false;
    }

    public String toWire()
        throws Exception
    {
        return Wired.to(
                Wired.map(
                        "c", clusters(),
                        "a", algorithms(),
                        "m", models(),
                        "t", tasks(),
                        "p", profiles()
                    ) );
    }

    public Object fromWire( String s )
        throws Exception
    {
        Map m = (Map) Wired.from(
                Wired.map(
                        "c", new MapNode( this, "clusters", new Cluster() ),
                        "a", new MapNode( this, "algorithms", new Algorithm() ),
                        "m", new MapNode( this, "models", new Model() ),
                        "p", new MapNode( this, "profiles", new Profile() ),
                        "t", new MapNode( this, "tasks", new Task() ) ), s );

        m_clusters = (MapNode) m.get( "c" );
        m_algorithms = (MapNode) m.get( "a" );
        m_models = (MapNode) m.get( "m" );
        m_profiles = (MapNode) m.get( "p" );
        m_tasks = (MapNode) m.get( "t" );
        return this;
    }


    protected long m_maxTaskRunTime;

    public void setName( String name )
    {
        m_name = name;
    }

    public void changePassword( String pass )
    {
        m_passwordHash = Global.createMD5( pass );
    }

    public boolean admin()
    {
        return m_admin;
    }

    public void setAdmin( boolean admin )
    {
        m_admin = admin;
    }
}
