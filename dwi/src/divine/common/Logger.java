// -*- mode: java; c-basic-offset: 4 -*- 
package divine.common;
import divine.common.Wired;
import java.util.Date;

public class Logger
{
    /* this class is for static members only */
    private Logger() {}

    private static int maxPriority = 16;
    private static String appName = "<unknown>";

    public static void setMaxPriority( int priority )
    {
        maxPriority = priority;
    }

    private static ThreadLocal< Integer > disabled = new ThreadLocal() {
        protected synchronized Object initialValue() {
            return new Integer(0);
        }
    };

    public static void disable() { disabled.set( disabled.get() + 1 ); }
    public static void enable() { disabled.set( disabled.get() - 1 ); }

    public static void log( int priority, String message )
    {
	if ( disabled.get() > 0 ) return;
        // Don't log unimportant messages
        if ( priority > maxPriority )
            return ;

        Date now = new Date();
        String pref = "|" + now.toString() + "| " + "(" + appName + ") [" + priority + "]: ";
        String lead = pref.replaceAll( ".", " " ).replaceFirst( "..$", ": " );
        String[] parts = message.split( "\n", 2 );
        System.err.println( pref + parts[0] );
        try {
            if (parts.length == 2 )
                System.err.print( Wired.prefix( lead, parts[1] ) );
        } catch (Exception e) {
            System.err.println( "exception caught processing log output" );
        }
    }

    public static void log( Exception ex )
    {
        ex.printStackTrace();
        /* if ( maxPriority == 0 )
            return ;
        System.err.println( "[E] type    " + ex.getClass().toString() );
        System.err.println( "    message " + ex.getMessage() );
        System.err.println( "    stacktrace follows" );
        for ( StackTraceElement s : ex.getStackTrace() )
        {
            System.err.println( "     " + s.getMethodName() );
            System.err.println( "      in " + s.getClassName() );
            System.err.println( "      on line " + s.getLineNumber() );
            } */
    }

   public static void setAppname( String a )
   {
      appName = a;
   }

}
