package divine.common;

/**
 * Provides often used static methods;
 *
 * @author xforejt
 */
public class Global
{

    /** Creates a new instance of Global */
    private Global()
    {
    }

    public static String createMD5( String key ) {
        try
        {
            byte[] intext = key.getBytes();
            StringBuffer sb = new StringBuffer();
            java.security.MessageDigest md5 = java.security.MessageDigest.getInstance( "MD5" );
            byte[] md5rslt = md5.digest( intext );
            for ( int i = 0 ; i < md5rslt.length ; i++ )
            {

                int val = 0xff & ( ( int ) md5rslt[ i ] );
                if ( val < 16 )
                    sb.append( "0" );
                sb.append( Integer.toHexString( val ) );
            }
            String hashed_key = sb.toString();
            //System.out.println(hashed_key);
            return hashed_key;
        }
        catch ( Exception e2 )
        {
            System.out.println( e2 );
            return null;
        }
    }
}
