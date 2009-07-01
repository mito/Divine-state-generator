// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;
import java.net.InetAddress;
import javax.net.ssl.SSLSocketFactory;
import javax.swing.*;
import java.util.*;
import java.io.*;
import javax.net.ssl.SSLSocket;

import divine.common.Logger;
import divine.common.User;
import divine.common.Request;
import divine.common.Wired;
import divine.common.Protocol;

public class LoginDialog extends javax.swing.JDialog {
    // XXX port number.
    private MainWindow client;
    private boolean loggedIn = false;
    protected User m_user;
    protected int m_port;

    public User user()
    {
        return m_user;
    }

    public LoginDialog( MainWindow parent, int port )
    {
        super( parent, true );
        this.client = parent;
        initComponents();
        m_port = port;
        this.getRootPane().setDefaultButton( this.btnLogin );
    }

    private void btnLoginActionPerformed( java.awt.event.ActionEvent evt ) {
        if ( !this.loggedIn ) {
            try
            {
                InetAddress adr = InetAddress.getByName( this.txtHost.getText() );
                SSLSocket sock = ( SSLSocket )
                    SSLSocketFactory.getDefault().createSocket( adr, m_port );
                client.client().setSocket( sock );
                String user = new String( txtUser.getText() );
                String pass = new String( txtPassword.getPassword() );

                Map< String, Object > m = client.client().request( "login",
                        Wired.map( "user", user, "pass", pass,
                                "version", Protocol.version().toString() ) );

                if ( m.get( "status" ).equals( "success" ) ) {
                    m_user = new User();
                    m_user.setNameInternal( user );
                    setVisible( false );
                } else {
                    JOptionPane.showMessageDialog( this,
                            "Could not login to the server, maybe name or"
                            + " password is wrong; the server said: "
                            + m.get( "status" ).toString(), "Login failed",
                            javax.swing.JOptionPane.ERROR_MESSAGE );
                }
            }
            catch ( Exception e )
            {
                JOptionPane.showMessageDialog( this,
                      "Could not login to the server:\n" + e.getMessage(),
                      "Login failed", javax.swing.JOptionPane.ERROR_MESSAGE );
                Logger.log( e );
            }
        }
    }

    /**
     * Returns the name filled in the user textbox
     */
    public String getUsername()
    {
        return this.txtUser.getText();
    }

    /**
     * Returns the name filled in the server textbox
     */
    public String getServername()
    {
        return this.txtHost.getText();
    }

    private void initComponents()
    {
        Icon logo = new ImageIcon( new File( client.client().imageDir(),
                        "logo.png" ).getAbsolutePath() );

        lblLogo = new JLabel();
        lblHost = new JLabel();
        lblUser = new JLabel();
        lblPassword = new JLabel();
        txtPassword = new JPasswordField();
        btnLogin = new JButton();
        btnExit = new JButton();
        txtHost = new JTextField();
        txtUser = new JTextField();

        getContentPane().setLayout( null );

        setDefaultCloseOperation( javax.swing.WindowConstants.DISPOSE_ON_CLOSE );
        setTitle( "Login to DiVinE" );
        setBackground( java.awt.Color.white );
        setCursor( new java.awt.Cursor( java.awt.Cursor.DEFAULT_CURSOR ) );
        setFocusCycleRoot( false );
        lblHost.setText( "Host:" );
        getContentPane().add( lblHost );
        lblHost.setBounds( 104, 10, 80, 20 );

        lblUser.setText( "User:" );
        getContentPane().add( lblUser );
        lblUser.setBounds( 104, 30, 80, 20 );

        lblPassword.setText( "Password:" );
        getContentPane().add( lblPassword );
        lblPassword.setBounds( 104, 50, 80, 20 );

        txtPassword.setEchoChar( '#' );
        getContentPane().add( txtPassword );
        txtPassword.setBounds( 184, 50, 210, 19 );

        btnLogin.setText( "Login" );
        btnLogin.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent evt )
                {
                    btnLoginActionPerformed( evt );
                }
            } );

        btnExit.setText( "Quit" );
        btnExit.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent e ) {
                    setVisible( false );
                }
            } );

        getContentPane().add( lblLogo );
        getContentPane().add( btnLogin );
        getContentPane().add( btnExit );
        lblLogo.setIcon( logo );
        lblLogo.setBounds( 10, 10, 84, 81 );

        btnLogin.setBounds( 224, 80, 80, 20 );
        btnExit.setBounds( 314, 80, 80, 20 );

        txtHost.setText( "anna.fi.muni.cz" );
        txtUser.setText( "" );
        txtPassword.setText( "" );
        getContentPane().add( txtHost );
        txtHost.setBounds( 184, 10, 210, 19 );

        getContentPane().add( txtUser );
        txtUser.setBounds( 184, 30, 210, 19 );

        java.awt.Dimension screenSize = java.awt.Toolkit.getDefaultToolkit().getScreenSize();
        setBounds( ( screenSize.width - 413 ) / 2,
                ( screenSize.height - 134 ) / 2, 413, 134 );
        addWindowListener( new java.awt.event.WindowAdapter() {
                public void windowOpened( java.awt.event.WindowEvent evt ) {
                    txtUser.grabFocus();
                }
            } );
    }

    private JButton btnLogin;
    private JButton btnExit;
    private JLabel lblHost;
    private JLabel lblPassword;
    private JLabel lblUser;
    private JLabel lblLogo;
    private JTextField txtHost;
    private JPasswordField txtPassword;
    private JTextField txtUser;

}
