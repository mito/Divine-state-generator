package divine.client;
import divine.common.*;
import java.util.*;
import javax.swing.*;
import java.awt.Component;

public class ProfileEditor
    extends MainWindow.NodePanel
{
    private Profile m_profile;
    protected MainWindow m_client;
    public Profile profile() { return m_profile; }

    public ProfileEditor( MainWindow c, Profile p )
    {
        m_client = c;
        initComponents();
        update( p );
    }

    public boolean update( Node elem )
    {
        if ( !( elem instanceof Profile ) )
            return false;

        return true;
    }

    private JButton bSave;
    private JButton bReset;
    private JButton bClone;
    private JToolBar toolbar;

    private void initComponents()
    {
        toolbar = new javax.swing.JToolBar();

        setLayout( new java.awt.BorderLayout() );

        bSave = new JButton();
        bSave.setText( "Save Profile" );
        bSave.addActionListener(
              new java.awt.event.ActionListener()
              {
                 public void actionPerformed( java.awt.event.ActionEvent evt )
                 {
                    Logger.log( 4, "we pretend to save the profile" );
                 }
              }
           );

        toolbar.add( bSave );

        bReset = new JButton();
        bReset.setText( "Reset Profile" );
        bReset.addActionListener(
              new java.awt.event.ActionListener()
              {
                 public void actionPerformed( java.awt.event.ActionEvent evt )
                 {
                    Logger.log( 4, "we pretend to reset the profile" );
                 }
              }
           );

        toolbar.add( bReset );

        add( toolbar, java.awt.BorderLayout.NORTH );

    }

}
