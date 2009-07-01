// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;

import java.util.*;
import java.util.regex.*;
import java.lang.*;
import java.io.*;

public class Algorithm extends Node
    implements Cloneable, Wired.Interface
{
    public Object clone() { return super.clone(); }
    public Algorithm self() { return this; }

    public Algorithm()
    {
        m_name = "algorithm";
        m_description = "";
    }

    public Algorithm( String n, String d )
    {
        m_name = n;
        m_description = d;
    }

    protected String m_description;
    protected String m_executable;
    protected String m_shortDescription;
    protected boolean m_needsProperty;

    public String description() { return this.m_description; }
    public String executable() { return m_executable; }

    protected int m_inputFormat = 0;

    public boolean supportsFormat( int format )
    {
        return ( format & this.m_inputFormat ) != 0;
    }

    public boolean canHandle( Property p ) {
        if ( p == null ) return false;
        if ( needsProperty()
             && p.type() == Property.Type.StateSpace )
            return false;
        if ( needsProperty()
             && p.type() == Property.Type.Safety )
            return false;
        return true;
    }

    public boolean needsProperty() { return m_needsProperty; }
    public String shortDescription() { return m_shortDescription; }

    public Object fromWire( String s ) // server from file, client from server
                                       // *never* server from client
        throws Exception
    {
        Map m = (Map) Wired.from(
                Wired.map(
                        "description", "", "executable", "", "needs-property", "",
                        "short", "" ), s );
        m_description = (String) m.get( "description" );
        m_shortDescription = (String) m.get( "short" );
        m_executable = (String) m.get( "executable" );
        m_needsProperty = false;
        if ( ( (String) m.get( "needs-property" ) ).equals( "yes" ) )
            m_needsProperty = true;
        return self();
    }

    public String toWire() // to client or to file
        throws Exception
    {
        return Wired.to(
                Wired.map(
                        "description", description(),
                        "short", shortDescription(),
                        "needs-property", m_needsProperty ? "yes" : "no",
                        "executable", executable()
                    ) );
    }

    public boolean equals( Object o ) {
        if ( o instanceof Algorithm )
            return ( (Algorithm) o ).name().equals( name() );
        throw new ClassCastException();
    }

}
