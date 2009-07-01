package divine.common;
import java.util.regex.*;
import java.util.Map;
import java.util.TreeMap;
import java.util.List;
import java.util.Arrays;

public class Model extends Node implements Wired.Interface, Cloneable
{
   protected String m_text;
   protected MapNode< Property > m_properties;
   
   public Model()
   {
      super( null, "Model" );
      m_properties = new MapNode< Property >( this, "properties" );
      m_text = "";
      // statespace and safety properties are always applicable
      Property ssp = new Property();
      ssp.setType( Property.Type.StateSpace );
      ssp.setNameInternal( "State Space" );
      addProperty( ssp );
      Property safety = new Property();
      safety.setType( Property.Type.Safety );
      safety.setNameInternal( "Safety" );
      addProperty( safety );
   }
   
   public Object clone() {
       Model m = (Model) super.clone();
       m.m_properties = (MapNode) m_properties.clone();
       m.m_properties.setParentInternal( m );
       return m;
   }
    
   public Map< String, Node > children() {
      return nodeMap( m_properties );
   }

   public enum Type { DVE, PML; }

   public String text() { return m_text; }
   public void setText( String t ) {
       setDirty();
       m_text = t;
       updatePropertyProcess();
   }
   
   protected class SyntaxCheck extends Cluster.Executable {
      protected String m_data;
      protected Property m_property;
      
      public SyntaxCheck( Property p ) { m_property = p; }
      public String data() { return m_data; }
      public void data( String s ) {
         m_data = s;
      }
      public void run( Cluster c ) {
         try {
            StringBuffer b = new StringBuffer();
            b.append( "divine.dwi-syntax" );
            boolean p = ( m_property == null );
            execute( c, Wired.to(
                        Wired.map(
                              "command", b.toString(),
                              "property-type",
                              p ? "none" : m_property.typeString(),
                              "property", p ? "" : m_property.text(),
                              "model", text(),
                              "model-type", typeString() ) ) );
         } catch ( Exception e ) { Logger.log( e ); }
      }
   }

   public Map< String, String > syntaxCheck( Cluster c, Property p )
      throws Exception
   {
      SyntaxCheck sc = new SyntaxCheck( p );
      sc.run( c );
      Map< String, String > r = new TreeMap(),
         m = (Map) Wired.from( Wired.mapTemplate( "" ), sc.data() );
      if ( m.get( "ok" ) != null ) r.put( "ok", m.get( "ok" ) );
      if ( m.get( "error" ) != null ) r.put( "error", m.get( "error" ) );
      return r;
   }
   
   public MapNode< Property > properties() { return m_properties; }

   public void addProperty( Property p ) {
      m_properties.addUnique( p );
   }

   protected Type m_type = Type.DVE;

   public Type type() { return m_type; }
   public void setType( Type t ) { setDirty(); m_type = t; }

   public static String typeToString( Type t ) {
      if ( t == Type.DVE ) return "dve";
      if ( t == Type.PML ) return "pml";
      return "null";
   }

   public static Type typeFromString( String s ) {
      if ( s.equals( "dve" ) ) return Type.DVE;
      if ( s.equals( "pml" ) ) return Type.PML;
      return null;
   }

   public String typeString() { return typeToString( m_type ); }

   public boolean hasProperty() { return !properties().isEmpty(); }
   public boolean hasPropertyProcess() {
      if ( type() == Type.DVE ) {
         Pattern p = Pattern.compile( "system .*? property" );
         return p.matcher( text() ).find();
      } else if ( type() == Type.PML ) {
         return text().contains( "never" );
      }
      return false;
   }

   public void updatePropertyProcess() {
       Property pp = properties().get( "Property Process" );
       if ( hasPropertyProcess() && pp == null ) {
           pp = new Property();
           pp.setNameInternal( "Property Process" );
           pp.setType( Property.Type.PropertyProcess );
           addProperty( pp );
       }
       if ( !hasPropertyProcess() && pp != null ) {
           pp.detach();
       }
   }

   public Object fromWire( String s )
      throws Exception
   {
       Map m = wireRead( s,
                         "type", "", "t", "", "p",
                         new MapNode( this, "properties", new Property() ) );

      m_text = (String) m.get( "t" );
      m_type = typeFromString( (String) m.get( "type" ) );
      m_properties = (MapNode) m.get( "p" );

      return this;
   }

   public String toWire()
      throws Exception
   {
      return wireWrite(
            "type", typeToString( m_type ),
            "t", text(),
            "p", properties() );
   }

}
