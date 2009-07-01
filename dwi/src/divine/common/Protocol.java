// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;
import java.util.*;
import java.util.regex.*;
import java.io.*;

public class Protocol {

    static public Integer version() {
        return 4;
    }

    public Protocol()
    {
        Logger.log( 1, "initialising protocol" );
        Vector< String > strlist = new Vector();
        strlist.add( "" ); // empty string list

        Map< String, Object > empty = Wired.map();
        TreeMap< String, Object > resp = Wired.map( "status", "success" );

        makeRequest( "login", Wired.map( "user", "", "pass", "", "version", "" ), resp );

        // XXX error/ok in response?
        makeRequest( "query-user-data", empty, Wired.map( "status", "success",
                                                          "data", new User() ) );
        makeRequest( "query-changes", empty, Wired.map( "status", "success", "changes",
                        Wired.mapTemplate( "" ) ) );
        makeRequest( "store-changes", Wired.map( "changes",
                        Wired.mapTemplate( "" ) ), resp );

        makeRequest( "logout", empty, resp );
        makeRequest( "store-model", Wired.map(
                                              "name", new String(),
                                              "model", new Model() ), resp );
        makeRequest( "delete-model", Wired.map( "model", new String() ), resp );

        makeRequest( "execute-job", Wired.map( "path", new Node.Path() ), resp );
        makeRequest( "abort-job", Wired.map( "path", new Node.Path() ), resp );

        makeRequest( "query-tasks", Wired.map( "task", "",
                        "model", "", "property", "" ), 
                     Wired.map( "status", "success",
                                "task", Wired.list( new Task() ) ) );

        makeRequest( "check-syntax", Wired.map( "model", "", "property", "" ), resp );
    }

    protected void makeRequest( String id,
            Map< String, Object > args, Map< String, Object > resp )
    {
        m_requests.put( id, new Request( id, args, resp ) );
    }

    protected Map< String, Request > m_requests = new TreeMap();

    public Request find( String id ) {
        return m_requests.get( id );
    }

    public void setHandler( String what, Object ho, String h )
    {
        find( what ).setHandler( ho, h );
    }

    public void handle( Reader _in, Writer out ) throws Exception
    {
        BufferedReader in = new BufferedReader( _in );
        in.mark( 512 );
        String s = in.readLine();
        if (s == null)
            throw new Exception( "EOF while reading from socket" );
        Matcher m = Pattern.compile( "R: (.*)" ).matcher( s );
        in.reset();
        if (m.matches()) {
            // Logger.log( 4, "Request.handle encountered R: " + m.group( 1 ) );
            find( m.group( 1 ) ).doHandle( in, out );
        } else
            throw new Exception( "could not decode request: " + s );
    }

}
