Public Class Instalar1

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        If Me.ListBox1.SelectedIndex > -1 Then
            Select Case ListBox1.SelectedIndex
                Case 0
                    CType(Me.ParentForm, Form1).ProcesarX36G(False)
                Case 1
                    CType(Me.ParentForm, Form1).ProcesarX36G(True)
                Case 2
                    If My.Computer.Info.OSVersion(0) = "6" Then CType(Me.ParentForm, Form1).ProcesarX45() Else CType(Me.ParentForm, Form1).ProcesarX36M()
                Case Else
                    CType(Me.ParentForm, Form1).ProcesarX45()
            End Select
        End If
    End Sub

    Private Sub ListBox1_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ListBox1.SelectedIndexChanged
        If Me.ListBox1.SelectedIndex > -1 Then Me.Button1.Enabled = True Else Me.Button1.Enabled = False
    End Sub

    Private Sub Instalar1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Select Case (System.Globalization.CultureInfo.CurrentCulture.LCID And &HFF)
            Case &HC
                'nombre = "french.xml"
            Case &H10
                'nombre = "italian.xml"
            Case &HA
                Label1.Text = "Selecciona el joystick que vas a instalar y dale a continuar:"
                Label2.Text = "NOTA: Enchufa el joystick antes de continuar"
                Button1.Text = "Continuar"
            Case &H7
                'nombre = "german.xml"
        End Select
        If My.Computer.Info.OSVersion(0) = "6" Then
            ListBox1.Items.RemoveAt(2)
        End If
    End Sub
End Class
