package divine.common;

import java.util.*;
import java.util.regex.*;
import java.io.*;
import java.lang.*;
import java.lang.reflect.*;

public class Wired {

    // (de)serialisation helpers

    public static String prefix( String pre, String b )
        throws Exception
    {
        StringWriter sw = new StringWriter();
        PrintWriter outbuf = new PrintWriter( sw );
        BufferedReader r = new BufferedReader( new StringReader( b ) );
        String l;
        while ( ( l = r.readLine() )  != null ) {
            outbuf.println( pre + l );
        }
        return sw.toString();
    }

    public static String prefixLines( String p, String b )
        throws Exception
    { return prefix( p, b ); }

    public static Map< String, String > unprefix( String pat, String in )
        throws Exception
    {
        // Logger.log( 6, "unprefix started: " + pat );
        Map< String, StringBuffer > work = new TreeMap();
        Map< String, String > ret = new TreeMap();
        BufferedReader inr = new BufferedReader( new StringReader( in ) );
        Pattern p = Pattern.compile( "^(" + pat + ")(.*)" );
        String l;
        while ( ( l = inr.readLine() )  != null ) {
            Matcher m = p.matcher( l );
            if ( m.matches() ) {
                boolean first = false;
                String key = m.group( 1 );
                if ( !work.containsKey( key ) ) {
                    work.put( key, new StringBuffer() );
                    first = true;
                }
                StringBuffer b = work.get( key );
                if ( !first )
                    b.append( "\n" );
                b.append( m.group( 2 ) );
            } else
                throw new Exception( "unprefix error in '" + l + "'" );
        }
        for ( String k : work.keySet() )
            ret.put( k, work.get( k ).toString() );
        // Logger.log( 6, "unprefix done" );
        return ret;
    }

    public interface Interface {
        public String toWire() throws Exception;
        public Object fromWire( String s ) throws Exception;
    }

    public static TreeMap< String, Object > map( Object... ola ) {
        List< Object > ol = Arrays.asList( ola );
        TreeMap< String, Object > a = new TreeMap();
        for ( int i = 0; i < ol.size(); i += 2 ) {
            Object obj = ol.get( i + 1 );
            a.put( ( String ) ol.get( i ), obj );
        }
        return a;
    }

    public static List list( Object... ola ) {
        return new Vector( Arrays.asList( ola ) );
    }

    public static Map mapTemplate( Object def, Object... rest ) {
        Map m = map( rest );
        m.put( "fromWire-template-default", def );
        return m;
    }

    public static Interface mapProxy( Map m )
    {
        return proxy( Map.class, new MapHandler( m ) );
    }

    public static Interface listProxy( List l )
    {
        return proxy( List.class, new ListHandler( l ) );
    }

    public static Interface proxy( Class c, InvocationHandler h )
    {
        return (Interface) Proxy.newProxyInstance( Wired.class.getClassLoader(),
                new Class[] { Interface.class, c }, h );
    }

   public static MapNode loadMapNode( Node p, String n, Map< String, Object > m, String k )
   {
      MapNode r = new MapNode( p, n );
      if ( m.get( k ) != null )
         r.putAll( (Map) m.get( k ) );
      return r;
   }

    public static String to( Object o )
        throws Exception
    {
        if ( o == null )
            return "!";
        if ( o.getClass() == String.class )
            return (String) o;
        if ( o.getClass() == Integer.class )
            return String.valueOf( (Integer) o );
        Interface i = makeInterface( o );
        if ( i == null )
            return "!";
        return i.toWire();
    }

    public static Object from( Object t, String s )
        throws Exception
    {
        // Logger.log( 6, "parsing: " + t.getClass().toString() );
        if ( t == null )
            return null;
        if ( t.getClass() == String.class )
            return s;
        Object x = t.getClass().getDeclaredMethod( "clone" ).invoke( t );
        Interface y = makeInterface( x );
        if ( y == null )
            return null;
        return y.fromWire( s );
        // return x;
    }

    public static abstract class Handler implements InvocationHandler, Interface
    {
        Object m_data;

        public Handler( Object o ) { m_data = o; }

        public Object invoke( Object prox, Method m, Object[] args )
            throws Exception
        {
            if (m.getDeclaringClass() != Interface.class)
                return m.invoke( m_data, args );
            else if ( m.getName().equals( "toWire" ) )
                return toWire();
            else if ( m.getName().equals( "fromWire" ) ) {
                return fromWire( (String) args[0] );
            } else {
                throw new Exception( "Handler called with bad method" );
            }
        }
    }

    public static class MapHandler extends Handler
    {
        public MapHandler( Object o ) { super( o ); }
        public Map map() { return (Map) m_data; }

        public String toWire() throws Exception {
            StringBuffer b = new StringBuffer();
            int i = 0;
            for ( Object k : map().keySet() ) {
                Object o = map().get( k );
                b.append( prefixLines( (String) k + ": ",
                                o == null ? "<null>" : Wired.to( o ) ) );
                ++i;
            }
            if ( b.toString().equals( "" ) )
                return "!";
            return b.toString();
        }

        public Object fromWire( String s )
            throws Exception
        {
            Map< String, Object > template = (Map) map().getClass().getDeclaredMethod(
                    "clone" ).invoke( map() );
            map().remove( "fromWire-template-default" );
            // map().clear();

            if ( s.equals( "!" ) || s.equals( "!\n" ) ) {
                return map();
            } else {
                Map< String, String > argm = unprefix( ".+?: ", s );
            
                Pattern p = Pattern.compile( "(.+?): " );
                for ( String k : argm.keySet() ) {
                    Matcher m = p.matcher( k );
                    if ( m.matches() ) {
                        String key = m.group( 1 );
                        /* Logger.log( 4, "parsing value " + key
                               + ":\n'" + argm.get( k ) + "'" ); */
                        Object t = template.get( key );
                        if ( t == null ) {
                            t = template.get( "fromWire-template-default" );
                            if ( t == null )
                                throw new Exception( "unknown parameter '"
                                        + key + "' while reading and no default" );
                        }
                        map().put( key, Wired.from( t, argm.get( k ) ) );
                    }
                }
            }
            return map();
        }
    }
    

    public static class ListHandler extends Handler
    {
        public ListHandler( Object o ) { super( o ); }
        public List list() { return (List) m_data; }

        public Object fromWire( String s ) throws Exception {

            Object first = list().get( 0 );
            if (first == null)
                throw new Exception( "fromWire called on a typeless list" );

            Vector vec = new Vector();
            Map< String, String > m = unprefix( "[0-9]+: ", s );
            Pattern p = Pattern.compile( "([0-9]+): " );
            for ( String k : m.keySet() ) {
                Matcher match = p.matcher( k );
                if ( match.matches() ) {
                    Object o = Wired.from( first, m.get( k ) );
                    int idx = Integer.parseInt( match.group( 1 ) );
                    if ( idx > vec.size() - 1 )
                        vec.setSize( idx + 1 );
                    vec.set( idx, o );
                }
            }

            list().clear();
            list().addAll( vec );
            return list();
        }

        public String toWire() throws Exception {
            StringBuffer b = new StringBuffer();
            int i = 0;
            for ( Object o : list() ) {
                b.append( prefixLines( String.valueOf( i ) + ": ",
                                o == null ? "null" : Wired.to( o ) ) );
                ++i;
            }
            return b.toString();
        }

    }

    public static Interface makeInterface( Object o )
    {
        if ( o instanceof Interface )
            return (Interface) o;

        if ( o instanceof Map )
            return mapProxy( (Map) o );
        if ( o instanceof List ) 
            return listProxy( (List) o );

        return null;
    }

}
