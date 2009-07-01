package divine.common;
import java.util.concurrent.ConcurrentHashMap;
import java.util.TreeMap;
import java.util.Map;
import java.util.List;
import java.util.Vector;
import java.util.Arrays;

public class Node implements Cloneable
{
   protected Node m_parent;
   protected String m_name;
   protected boolean m_dirty, m_changed, m_deleted;

   public static class Path extends Vector< String >
      implements Wired.Interface, Cloneable {
      public Object fromWire( String s )
         throws Exception
      {
         while ( s.length() > 0 ) {
            int chop = s.indexOf( ':' );
            if ( chop == -1 ) chop = s.length();
            add( s.substring( 0, chop ) );
            s = s.substring( Math.min( chop + 1, s.length() ) );
         }

         return this;
      }

      public String toWire()
         throws Exception
      {
         StringBuffer sb = new StringBuffer();
         for ( String s : this ) {
            sb.append( s );
            sb.append( ":" );
         }
         sb.deleteCharAt( sb.length() - 1 ); // kill the trailing
                                             // colon
         return sb.toString();
      }

      public Path parent() {
         Path ret = (Path) clone();
         if ( size() > 0 )
            ret.remove( size() - 1 );
         return ret;
      }

      public Path descended() {
         Path ret = (Path) clone();
         ret.remove( 0 );
         return ret;
      }

      public String last() { return get( size() - 1 ); }
      public String first() { return get( 0 ); }

      public Object clone()
      { return super.clone(); }

   }

   public Node() { m_dirty = true; m_changed = true; m_deleted = false; }

   public Node( Node p, String n ) {
      m_parent = p;
      m_name = n;
      m_dirty = true;
      m_changed = true;
      m_deleted = false;
   }

   public String name() { return m_name; }
   public String toString() { return getClass().getName() + ": " + name(); }
   public void setNameInternal( String s ) { m_name = s; setDirty(); }
   public void setParentInternal( Node n ) {
      m_parent = n;
      notifyParentChanged( parent() );
   }

   public void notifyParentChanged( Object o ) {}

   public Node parent() {
      return m_parent;
   }

   public Path path() {
      Path p;
      if ( parent() != null )
         p = (Path) parent().path().clone();
      else
         p = new Path();
      p.add( name() );
      return p;
   }

   public void setDeleted() {
       m_deleted = m_changed = m_dirty = true;
       for ( Node child : children().values() )
           child.setDeleted();
   }

   public void clearChanged() { m_changed = false; }
   public void setChanged() { m_changed = true; }
   public void setDirty() { m_dirty = true; setChanged(); }
   public void clearDirty() { m_dirty = false; }
   public boolean deleted() { return m_deleted; }
   public boolean changed() { return m_changed; }
   public boolean dirty() { return m_dirty; }

   public void clearDirtyRec() {
      clearDirty();
      for ( Node n : children().values() )
         n.clearDirtyRec();
   }

   public Map< String, Node > children() { return new TreeMap(); }

   public static TreeMap< String, Node > nodeMap( Node... ola ) {
      List< Node > ol = Arrays.asList( ola );
      TreeMap< String, Node > a = new TreeMap();
      for ( Node n : ol ) {
         a.put( n.name(), n );
      }
      return a;
   }

   public Node find( Path p ) {
      if ( p.isEmpty() ) return null;
      String n = p.first();
      p = p.descended();
      if ( ! n.equals( name() ) )
         return null;

      if ( p.isEmpty() )
         return this;

      // Logger.log( 4, name() + " looking for " + p.toString() );
      Node ch = children().get( p.first() );
      if ( ch == null ) {
         // Logger.log( 4, name() + " doesn't have any child called " + p.first() );
         return null;
      }
      return ch.find( p );
   }

   public void rename( String n ) {
      if ( parent() != null )
         parent().renameChild( this, n );
      else
         m_name = n;
   }

   public void detach() {
      if ( parent() != null )
         parent().detachChild( this );
      m_parent = null;
   }

   public void renameChild( Node n, String name ) {
      n.setNameInternal( name );
   }

   public void detachChild( Node n ) {} // do nothing

   // kill any deleted() nodes
   public void prune() {
      for ( Node c : children().values() ) {
         if ( c.deleted() )
            c.detach();
         else
            c.prune();
      }
   };

   public List< Node > childrenList() {
      List< Node > l = new Vector();
      for ( Node n : children().values() ) {
         l.add( n );
         l.addAll( n.childrenList() );
      }
      return l;
   }

   public Map< String, Node > dirtyMap() throws Exception {
      Map< String, Node > m = new TreeMap();
      for ( Node n : childrenList() ) {
         if ( n.dirty() )
            m.put( n.path().toWire(), n );
      }
      return m;
   }

    public String wireWrite( Object... rest ) throws Exception {
        if ( m_deleted ) return "!deleted";
        return Wired.to( Wired.map( rest ) );
    }

    public Map< String, Object > wireRead( String s, Object... rest )
        throws Exception
    {
        if ( s.equals( "!deleted" ) ) {
            setDeleted();
            return Wired.map( rest ); // give back defaults
        } else
            return (Map) Wired.from( Wired.map( rest ), s );
    }

   public int readInteger( int def, Map map, String key ) {
      try {
         return Integer.parseInt( (String) map.get( key ) );
      } catch ( Exception e ) {
         return def;
      }
   }

   // XXX generalize, flexibilize
    public void mergeNodeMap( Map< String, String > m ) throws Exception {
       // Logger.disable();
        for ( String path : m.keySet() ) {
            Path p = (Path) Wired.from( new Path(), path );
            
            Logger.log( 6, "merging node change at: " + p.toWire() );
            Node n = find( p );
            
            if ( n == null && ! m.get( path ).equals( "!deleted" ) ) {
               // Logger.log( 6, "node doesn't exist, looking for parent: " + p.parent().toWire() );
                Node maybemap = find( p.parent() );
                // if ( maybemap != null )
                //   Logger.log( 6, "found, looking if it's a MapNode: " + p.parent().toWire() );
                if ( maybemap instanceof MapNode ) {
                    // Logger.log( 6, "calling merge child for: " + p.toWire() );
                    MapNode map = (MapNode) maybemap;
                    map.mergeChild( p.last(), m.get( path ) );
                }
            } else {
                if ( n != null && m.get( path ).equals( "!deleted" ) ) {
                    n.setDeleted();
                } else if ( n != null ) {
                   // Logger.log( 6, "node exists, refreshing: " + p.toWire() );
                    ((Wired.Interface) n).fromWire( m.get( path ) );
                    n.setChanged();
                }
            }
        }
        // Logger.enable();
   }

   public Object clone()
   // throws CloneNotSupportedException
   {
      try {
         return super.clone();
      } catch ( CloneNotSupportedException x ) {
         return null;
      }
   }

}

