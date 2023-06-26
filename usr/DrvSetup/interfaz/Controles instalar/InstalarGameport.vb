Public Class InstalarGameport
    Private Declare Ansi Function PrepararInterfaces Lib "drvapi.dll" (ByVal hwnd As IntPtr) As Byte

    Private Sub InstalarTeclado_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Dim ret As Byte = PrepararInterfaces(ListBox1.Handle)
        Select Case (System.Globalization.CultureInfo.CurrentCulture.LCID And &HFF)
            Case &HC
                'nombre = "french.xml"
            Case &H10
                'nombre = "italian.xml"
            Case &HA
                Label1.Text = "Selecciona el puerto de juegos usado por el X36F:"
                Button1.Text = "Continuar"
                If ret = 0 Then
                    ListBox1.Items.Add("Ningún dispositivo encontrado")
                    Button1.Text = "Salir"
                End If
            Case &H7
                'nombre = "german.xml"
            Case Else
                If ret = 0 Then
                    ListBox1.Items.Add("No devices found")
                    Button1.Text = "Exit"
                End If
        End Select
    End Sub

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        If Button1.Text = "Exit" Then
            Me.ParentForm.Close()
        Else
            If ListBox1.SelectedIndex > -1 Then
                CType(Me.ParentForm, Form1).ProcesarGameports(ListBox1.SelectedIndex)
            End If
        End If
    End Sub

    Private Sub ListBox1_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ListBox1.SelectedIndexChanged
        If Me.ListBox1.SelectedIndex > -1 Then Me.Button1.Enabled = True Else Me.Button1.Enabled = False
    End Sub
End Class
