package divine.common;
import java.util.concurrent.ConcurrentHashMap;
import java.util.TreeMap;
import java.util.Map;
import java.util.List;
import java.util.Vector;
import java.util.Set;
import java.util.Collection;

public class MapNode< E extends Node > extends Node implements Wired.Interface, Cloneable
{
   TreeMap< String, E > m_map;
   E m_prototype;

   public MapNode( Node p, String n, E proto ) {
      super( p, n );
      m_prototype = proto;
      m_map = new TreeMap();
   }

   public Object clone() {
      MapNode mn = (MapNode) super.clone();
      mn.m_map = new TreeMap();
      if ( !isEmpty() )
          for ( E i : m_map.values() )
              mn.add( (E) i.clone() );
      return mn;
   }
    
   public void setPrototype( E proto ) { m_prototype = proto; }

   public MapNode( Node p, String n) {
      super( p, n );
      m_map = new TreeMap();
   }

   public MapNode( Node p, String n, Map< String, E > m ) {
      super( p, n );
      m_map = new TreeMap();
      putAll( m );
   }

   public Map< String, Node > children() {
      TreeMap< String, Node > r = new TreeMap();
      for ( Node n : m_map.values() )
         r.put( n.name(), n );
      return r;
   }

   public Collection< E > values() { return m_map.values(); }
   public Set< String > keySet() { return m_map.keySet(); }
   public int size() { return m_map.size(); }
   
   public E get( String k ) {
      return m_map.get( k );
   }

   public E any() {
       if ( isEmpty() )
           return null;
       return (E) values().toArray()[ 0 ];
   }

   public boolean isEmpty() { return m_map.isEmpty(); }

   public E getSome( String k ) {
       if ( get( k ) == null )
           return any();
       return get( k );
   }
   
   public E add( E o ) {
      if ( o == null )
         return null;
      return put( o.name(), o );
   }
   
   public E put( String s, E o ) {
      o.detach();
      o.setNameInternal( s );
      o.setParentInternal( this );
      return m_map.put( s, o );
   }
   
   public void putAll( Map< ? extends String, ? extends E > m ) {
      for ( String k : m.keySet() ) {
         m.get( k ).setNameInternal( k );
         add( m.get( k ) );
      }
   }

   public void replace( Map< ? extends String, ? extends E > m ) {
      clear();
      putAll( m );
   }
   
   public String uniqueName( String n ) { // O(n)
      String m = n;
      int i = 1;
      while ( get( m ) != null && ! get( m ).deleted() ) {
         m = n + " " + i;
         ++ i;
      }
      return m;
   }

   public void clear() { m_map.clear(); }

   public E addUnique( E o ) {
      o.detach();
      o.setNameInternal( uniqueName( o.name() ) );
      return add( o );
   }

   public void renameChild( Node o, String s ) {
      detachChild( o );
      o.setNameInternal( s );
      addUnique( (E) o );
   }

   public void detachChild( Node o ) {
      if ( o.equals( get( o.name() ) ) )
         m_map.remove( o.name() );
   }

   public void mergeChild( String s, String w ) {
      try {
         E o = (E) Wired.from( m_prototype, w );
         put( s, o );
      } catch( Exception e ) {
         Logger.log( e );
      }
   }

   public String toWire() throws Exception {
      return Wired.to( m_map );
   }

   public Object fromWire( String s ) throws Exception {
      m_map.clear();
      // Logger.log( 5, "MapNode.fromWire( " + s + " )" );
      // Logger.log( 5, "MapNode.fromWire prototype = " + m_prototype.toString() );
      putAll( (Map) Wired.from( Wired.mapTemplate( m_prototype ), s ) );
      return this;
   }

}

