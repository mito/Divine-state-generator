// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;

import java.util.*;
import javax.swing.*;
import java.awt.*;
import divine.common.*;

public class TaskEditor
    extends MainWindow.NodePanel
{
    protected Task m_task;
    protected Client m_client;
    protected Map< String, Algorithm > m_comboAlgorithms;
    protected int m_inUpdate = 0;

    public Task task() { return m_task; }

    public TaskEditor( Client client, Task t )
    {
        m_client = client;
        initComponents();

        m_comboAlgorithms = new TreeMap();

        update( t );
    }

    public void updateProperties() {
        m_inUpdate ++;
        TreeSet< String > cl = new TreeSet();
        if ( task().model() != null ) {
            cl.addAll( task().model().properties().keySet() );
        }
        comboProperty.setModel( new DefaultComboBoxModel( cl.toArray() ) );
        m_inUpdate --;
    }

    public void updateModels() {
        m_inUpdate ++;
        TreeSet< String > cl = new TreeSet();
        for ( Model m : task().user().models().values() ) {
            if ( m.properties().values().isEmpty() ) continue;
            Property p = (Property) ( m.properties().values().toArray() )[ 0 ];
            cl.add( m.name() );
        }
        comboModel.setModel( new DefaultComboBoxModel( cl.toArray() ) );
        m_inUpdate --;
    }

    public void updateAlgorithms() {
        m_inUpdate ++;
        TreeSet< String > cl = new TreeSet();
        for ( Algorithm a : task().user().algorithms().values() ) {
            if ( a.canHandle( task().property() ) ) {
                cl.add( a.description() );
                m_comboAlgorithms.put( a.description(), a );
            }
        }
        comboAlgorithm.setModel( new DefaultComboBoxModel( cl.toArray() ) );
        m_inUpdate --;
    }

    public void updateAlgorithm() {
        if ( task().algorithm() != null ) {
            String idx = task().algorithm().description();
            if ( !idx.equals( comboAlgorithm.getSelectedItem() ) )
                    comboAlgorithm.setSelectedItem( idx );
        }
    }

    public void updateProperty() {
        if ( task().property() != null ) {
            Logger.log( 4, "selecting " + task().property().name() );
            comboProperty.setSelectedItem( task().property().name() );
        }
    }

    public void updateModel() {
        if ( task().model() != null ) {
            Logger.log( 4, "selecting " + task().model().name() );
            comboModel.setSelectedItem( task().model().name() );
        }
    }

    public boolean update( Node e ) {
        if ( ! (e instanceof Task) ) 
            return false;

        Task t = ( Task ) e;
        if ( t.algorithm() == null || t.property() == null ) {
            Logger.log( 4, "skipping broken task " + t.name() );
            return false;
        }
        Logger.log( 4, "TaskEditor updating for " + t.name() );

        m_inUpdate ++;

        if ( m_task != t ) {
            m_task = t;
            updateModels();
            updateProperties();
            updateAlgorithms();
        }

        if ( selectedModel() != task().model() )
            updateModel();
        if ( selectedProperty() != task().property() )
            updateProperty();
        if ( selectedAlgorithm() != task().algorithm() )
            updateAlgorithm();

        setEnabledRec( this, task().editable() );

        m_inUpdate --;
        return true;
    }

    protected Model selectedModel() {
        return task().user().models().get( (String) comboModel.getSelectedItem() );
    }

    protected Property selectedProperty() {
        return task().model().properties().get( (String) comboProperty.getSelectedItem() );
    }

    protected Algorithm selectedAlgorithm() {
        return m_comboAlgorithms.get( (String) comboAlgorithm.getSelectedItem() );
    }

    public void rename() {
        if ( m_inUpdate > 0 ) return;
        m_client.renameObject( task(), task().canonicalName() );
    }

    private JComboBox comboAlgorithm, comboModel, comboProperty;
    private JLabel lModel, lProperty, lAlgorithm;

    private void initComponents()
    {
        comboModel = new javax.swing.JComboBox();
        comboProperty = new javax.swing.JComboBox();
        comboAlgorithm = new javax.swing.JComboBox();

        lModel = new JLabel( "Model: ", JLabel.TRAILING );
        lProperty = new JLabel( "Property: ", JLabel.TRAILING );
        lAlgorithm = new JLabel( "Algorithm: ", JLabel.TRAILING );

        comboModel.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent evt )
                {
                    if ( m_inUpdate > 0 ) return;
                    Model m = selectedModel();
                    Property p = (Property) ( m.properties().values().toArray() )[ 0 ];
                    if ( task().model() == m ) return;
                    if ( p != null )
                        task().setProperty( p );
                    updateProperties();
                    updateProperty();
                    rename();
                }
            } );

        comboProperty.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent evt )
                {
                    if ( m_inUpdate > 0 ) return;
                    if ( task().model() == null ) return;
                    Property p = selectedProperty();
                    if ( task().property() == p ) return;
                    task().setProperty( p );
                    updateAlgorithms();
                    updateAlgorithm();
                    rename();
                }
            } );


        comboAlgorithm.addActionListener( new java.awt.event.ActionListener() {
                public void actionPerformed( java.awt.event.ActionEvent evt )
                {
                    if ( m_inUpdate > 0 ) return;
                    if ( task().algorithm() == selectedAlgorithm() ) return;
                    task().setAlgorithm( selectedAlgorithm() );
                    rename();
                }
            } );

        setLayout( new SpringLayout() );

        add( lModel ); add( comboModel );
        add( lProperty ); add( comboProperty );
        add( lAlgorithm ); add( comboAlgorithm );

        JPanel space = new JPanel();
        space.setPreferredSize( new Dimension( 10, 1000 ) );
        add( space );
        space = new JPanel();
        space.setPreferredSize( new Dimension( 1000, 1000 ) );
        add( space );

        SpringUtilities.makeCompactGrid( this, 4, 2, 3, 3, 3, 3 );
    }

}
