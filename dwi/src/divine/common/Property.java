// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;
import java.util.Map;
import java.util.List;
import java.util.Arrays;

/**
   Properties are (usually) LTL formulae concerning some model.
 */

public class Property extends Node
   implements Wired.Interface, Cloneable
{
    protected String m_text;
    protected Type m_type = Type.LTL;

    public Object clone()
    {
        return super.clone();
    }
    
    public enum Type { LTL, StateSpace, PropertyProcess, Safety }

    public String typeString() { return typeToString( m_type ); }

    public static String typeToString( Type t ) {
        if (t == Type.LTL) return "ltl";
        if (t == Type.PropertyProcess) return "process";
        if (t == Type.StateSpace ) return "statespace";
        if (t == Type.Safety ) return "safety";
        return "null";
    }

    public static Type typeFromString( String s ) {
        if ( s.equals( "ltl" ) ) return Type.LTL;
        if ( s.equals( "process" ) ) return Type.PropertyProcess;
        if ( s.equals( "statespace" ) ) return Type.StateSpace;
        if ( s.equals( "safety" ) ) return Type.Safety;
        return null;
    }

    public Property()
    {
       super( null, "Property" );
       m_type = Type.StateSpace;
       m_text = "";
    }

    public Model model() { return (Model) parent().parent(); }

    public String text() { return m_text; }
    public void setText( String t ) { m_text = t; setDirty(); }

    public Type type() { return this.m_type; }
    public void setType( Type type ) { this.m_type = type; setDirty(); }

    public boolean typeValid() {
        if ( type() == Type.StateSpace ) return true;
        if ( type() == Type.Safety ) return true;
        if ( type() == Type.PropertyProcess
                && model().hasPropertyProcess() ) return true;
        if ( type() == Type.LTL && !model().hasPropertyProcess() ) return true;
        return false;
    }

    public String canonicalName() {
        if ( type() == Type.StateSpace )
            return "State Space";
        if ( type() == Type.Safety )
            return "Safety";
        if ( type() == Type.LTL )
            return "LTL Property";
        if ( type() == Type.PropertyProcess ) {
            if ( model().type() == Model.Type.DVE )
                return "Property Process";
            if ( model().type() == Model.Type.PML )
                return "Neverclaim";
        }
        return "Property";
    }

    public Object fromWire( String s )
        throws Exception
    {
        Map m = wireRead( s, "type", "", "tx", "" );
        m_type = typeFromString( (String) m.get( "type" ) );
        m_text = (String) m.get( "tx" );
        return this;
    }

    public String toWire()
        throws Exception
    {
        return wireWrite(
                "type", typeString(),
                "tx", text() );
    }

}

